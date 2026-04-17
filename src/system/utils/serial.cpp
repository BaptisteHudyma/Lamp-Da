#include "serial.h"

#include "constants.h"

#include "src/system/logic/behavior.h"

#include "src/system/power/power_handler.h"
#include "src/system/power/PDlib/power_delivery.h"
#include "src/system/power/charger.h"
#include "src/system/power/balancer.h"

#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/print.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/button.h"
#include "src/system/physical/fileSystem.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/statistics_handler.h"

namespace lampda {
namespace utils {
namespace serial {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

inline const char* boolToString(bool b) { return b ? "true" : "false"; }

void handleCommand(const platform::Inputs::Command& command)
{
  switch (utils::hash(command.data()))
  {
    case utils::hash("h"):
    case utils::hash("help"):
      {
        platform::lampda_print(
                "---Lamp-da CLI---\n"
                "h: this page\n"
                "v: hardware & software version\n"
                "t: return the lamp type\n"
                "id: return the board serial number\n"
                "stats: display the system use statistics\n"
                "bat: battery info/levels\n"
                "cinfo: charger infos\n"
                "ADC: values from the charger ADC\n"
                "PD: display the connected PD capabilities\n"
                "states: state machine states\n"
                "alerts: show all raised alerts\n"
                "i2c: start an i2c present check\n"
                "format-fs: format the whole file system (dangerous)\n"
                "DFU: clear this program from memory, enter update mode\n"
                "buttonTogg: change the button pin number for the next boot\n"
                "shutdown: force shutdown the lamp\n"
                "tasks: display a debug of task usages\n"
                "-----------------");
        break;
      }

    case utils::hash("v"):
      {
        platform::lampda_print(
                "hardware:%d.%d\n"
                "firmware:%d.%d\n"
                "base software:%d.%d\n"
                "user software:%d.%d",
                HARDWARE_VERSION_MAJOR,
                HARDWARE_VERSION_MINOR,
                EXPECTED_FIRMWARE_VERSION_MAJOR,
                EXPECTED_FIRMWARE_VERSION_MINOR,
                BASE_SOFTWARE_VERSION_MAJOR,
                BASE_SOFTWARE_VERSION_MINOR,
                USER_SOFTWARE_VERSION_MAJOR,
                USER_SOFTWARE_VERSION_MINOR);
        break;
      }

    case utils::hash("t"):
      {
#ifdef LMBD_LAMP_TYPE__INDEXABLE
        platform::lampda_print("indexable");
#else
#ifdef LMBD_LAMP_TYPE__SIMPLE
        platform::lampda_print("simple");
#else
#ifdef LMBD_LAMP_TYPE__CCT
        platform::lampda_print("cct");
#else
#error "Unspecified lamp type in CLI"
#endif /* LMBD_LAMP_TYPE__CCT */
#endif /* LMBD_LAMP_TYPE__SIMPLE */
#endif /* LMBD_LAMP_TYPE__INDEXABLE */
        break;
      }

    case utils::hash("id"):
      {
        platform::lampda_print("Serial number: %lu", platform::registers::get_device_serial_number());
        break;
      }

    case utils::hash("stats"):
      {
        logic::statistics::show();
        break;
      }

    case utils::hash("bat"):
      {
        const auto& balancerStatus = power::balancer::get_status();
        const bool areBalancerValueValid = balancerStatus.is_valid();

        if (areBalancerValueValid)
        {
          // print individual battery voltages
          for (uint8_t i = 0; i < batteryCount; ++i)
            platform::lampda_print("cell %d: %d mV, is balancing: %s",
                                   i,
                                   balancerStatus.batteryVoltages_mV[i],
                                   boolToString(balancerStatus.isBalancing[i]));
          platform::lampda_print("total (from balancer) %dmv\n", balancerStatus.stackVoltage_mV);
        }
        else
        {
          platform::lampda_print("balancer measurments not valid");
        }

        const auto& chargerStatus = power::charger::get_state();
        const bool areChargerValueValid = chargerStatus.areMeasuresOk;
        if (areChargerValueValid)
        {
          platform::lampda_print("total (from charger) %dmv", chargerStatus.batteryVoltage_mV);
        }
        else
        {
          platform::lampda_print("charger measurments not valid");
        }

        if (areChargerValueValid or areBalancerValueValid)
        {
          // print individual battery voltages
          platform::lampda_print(
                  "raw battery level:%.2f%%\n"
                  "battery level:%.2f%%\n"
                  "minimum cell level:%.2f%%",
                  physical::battery::get_level_percent(physical::battery::get_raw_battery_voltage_mv()) / 100.0,
                  physical::battery::get_battery_level() / 100.0,
                  physical::battery::get_battery_minimum_cell_level() / 100.0);
        }
        else
        {
          platform::lampda_print("Battery measurments not valid");
        }
        break;
      }

    case utils::hash("cinfo"):
      {
        const auto& chargerState = power::charger::get_state();
        if (chargerState.areMeasuresOk)
        {
          platform::lampda_print(
                  "is charge signal ok:%s\n"
                  "voltage on power rail:%dmV\n"
                  "input current:%dmA\n"
                  "battery voltage:%dmV\n"
                  "charge current:%dmA\n"
                  "is usb serial connected:%s\n"
                  "is charging:%s\n"
                  "is effec charging:%s\n"
                  "battery level:%.2f%%\n"
                  "-> charger status: %s",
                  boolToString(chargerState.isChargeOkSignalHigh),
                  chargerState.powerRail_mV,
                  chargerState.inputCurrent_mA,
                  chargerState.batteryVoltage_mV,
                  chargerState.chargeCurrent_mA,
                  boolToString(power::charger::is_vbus_signal_detected()),
                  boolToString(chargerState.is_charging()),
                  boolToString(chargerState.is_effectivly_charging()),
                  physical::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }
        else
        {
          platform::lampda_print(
                  "is charge signal ok:%s\n"
                  "Charger measurments are invalid !!\n"
                  "is usb serial connected:%s\n"
                  "battery level:%.2f%%\n"
                  "-> charger status: %s",
                  boolToString(chargerState.isChargeOkSignalHigh),
                  boolToString(power::charger::is_vbus_signal_detected()),
                  physical::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }

        // in case there is a software error, display it
        if (chargerState.status == power::charger::Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          platform::lampda_print("\t hardware error detail: \"%s\"", chargerState.hardwareErrorMessage.c_str());
        }
        if (chargerState.status == power::charger::Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          platform::lampda_print("\t software error detail: \"%s\"", chargerState.softwareErrorMessage.c_str());
        }
        break;
      }

    case utils::hash("alerts"):
      logic::alerts::show_all();
      break;

    case utils::hash("i2c"):
      {
        platform::lampda_print(
                "fusb detected : %d\n"
                "imu detected: %d\n"
                "balancer detected: %d\n"
                "charger detected: %d",
                platform::i2c::i2c_check_existence(0, platform::i2c::pdNegociationI2cAddress) == 0,
                platform::i2c::i2c_check_existence(0, platform::i2c::imuI2cAddress) == 0,
                platform::i2c::i2c_check_existence(0, platform::i2c::batteryBalancerI2cAddress) == 0,
                platform::i2c::i2c_check_existence(0, platform::i2c::chargeI2cAddress) == 0);
        break;
      }

    case utils::hash("ADC"):
      {
        const auto& chargerState = power::charger::get_state();
        if (chargerState.areMeasuresOk)
        {
          platform::lampda_print(
                  "Last update %dms\n"
                  "PowerRail voltage:%dmV\n"
                  "PowerRail current:%dmA\n"
                  "VBUS voltage:%dmA\n"
                  "Bat voltage:%dmV\n"
                  "Bat current:%dmA\n"
                  "Temperature:%.2fC",
                  chargerState.lastUpdateTime_ms,
                  chargerState.powerRail_mV,
                  chargerState.inputCurrent_mA,
                  power::powerDelivery::get_vbus_voltage(),
                  chargerState.batteryVoltage_mV,
                  chargerState.batteryCurrent_mA,
                  platform::registers::read_CPU_temperature_degreesC());
        }
        else
        {
          platform::lampda_print(
                  "Charger measurment are invalid !\n"
                  "Last update %dms\n"
                  "VBUS voltage:%dmA\n"
                  "Temperature:%.2fC",
                  chargerState.lastUpdateTime_ms,
                  power::powerDelivery::get_vbus_voltage(),
                  platform::registers::read_CPU_temperature_degreesC());
        }
        break;
      }

    case utils::hash("PD"):
      {
        power::powerDelivery::show_pd_status();
        const auto& pd = power::powerDelivery::get_available_pd();
        if (pd.empty())
        {
          platform::lampda_print("No power delivery capabilities");
        }
        else
        {
          platform::lampda_print("Power delivery profiles :");
          for (const auto& pdo: pd)
            platform::lampda_print("- %dmV, %dmA", pdo.voltage_mv, pdo.maxCurrent_mA);
        }
        break;
      }

    case utils::hash("states"):
      {
        platform::lampda_print("behavior machine state:%s. error msgs: %s",
                               logic::behavior::get_state().c_str(),
                               logic::behavior::get_error_state_message().c_str());
        platform::lampda_print("power state machine state: %s. error msgs: %s",
                               power::get_state().c_str(),
                               power::get_error_string().c_str());
        break;
      }

    case utils::hash("format-fs"):
      platform::lampda_print("clearing the whole file format");
      physical::fileSystem::clear_internal_fs();
      break;

    case utils::hash("DFU"):
      platform::registers::enter_serial_dfu();
      break;

    case utils::hash("buttonTogg"):
      {
        if (physical::button::get_button_pin() == platform::gpio::DigitalPin::GPIO::gpio3)
        {
          physical::button::set_button_pin(platform::gpio::DigitalPin::GPIO::gpio4);
          platform::lampda_print("Set button pin to gpio4");
        }
        else
        {
          physical::button::set_button_pin(platform::gpio::DigitalPin::GPIO::gpio3);
          platform::lampda_print("Set button pin to gpio3");
        }
        break;
      }

    case utils::hash("shutdown"):
      logic::behavior::internal::handle_shutdown_state();
      break;

    case utils::hash("tasks"):
      char buff[512];
      platform::threads::get_thread_debug(buff);
      platform::lampda_print("%s", buff);
      break;

    default:
      platform::lampda_print("unknown command: \'%s\'", command.data());
      platform::lampda_print("type h for available commands");
      break;
  }
}

void setup() { platform::init_prints(); }

void handleSerialEvents()
{
  const auto& inputs = platform::read_inputs();
  for (size_t i = 0; i < min<uint8_t>(platform::Inputs::maxCommands, inputs.commandCount); i++)
  {
    const platform::Inputs::Command& input = inputs.commandList[i];
    handleCommand(input);
  }
}

} // namespace serial
} // namespace utils
} // namespace lampda
