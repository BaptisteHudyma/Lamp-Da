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
#include "src/system/physical/fileSystem.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/statistics_handler.h"

namespace serial {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

inline const char* boolToString(bool b) { return b ? "true" : "false"; }

void handleCommand(const std::string& command)
{
  switch (utils::hash(command.c_str()))
  {
    case utils::hash("h"):
    case utils::hash("help"):
      {
        lampda_print(
                "---Lamp-da CLI---\n"
                "h: this page\n"
                "v: hardware & software version\n"
                "t: return the lamp type\n"
                "id: return the board serial number\n"
                "stats: display the system use statistics"
                "bat: battery info/levels\n"
                "cinfo: charger infos\n"
                "ADC: values from the charger ADC\n"
                "PD: display the connected PD capabilities\n"
                "states: state machine states\n"
                "alerts: show all raised alerts\n"
                "i2c: start an i2c present check\n"
                "format-fs: format the whole file system (dangerous)\n"
                "DFU: clear this program from memory, enter update mode\n"
                "tasks: display a debug of task usages\n"
                "-----------------");
        break;
      }

    case utils::hash("v"):
      {
        lampda_print(
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
        lampda_print("indexable");
#else
#ifdef LMBD_LAMP_TYPE__SIMPLE
        lampda_print("simple");
#else
#ifdef LMBD_LAMP_TYPE__CCT
        lampda_print("cct");
#else
#error "Unspecified lamp type in CLI"
#endif /* LMBD_LAMP_TYPE__CCT */
#endif /* LMBD_LAMP_TYPE__SIMPLE */
#endif /* LMBD_LAMP_TYPE__INDEXABLE */
        break;
      }

    case utils::hash("id"):
      {
        lampda_print("Serial number: %lu", get_device_serial_number());
        break;
      }

    case utils::hash("stats"):
      {
        statistics::show();
        break;
      }

    case utils::hash("bat"):
      {
        const auto& balancerStatus = balancer::get_status();
        const bool areBalancerValueValid = balancerStatus.is_valid();

        if (areBalancerValueValid)
        {
          // print individual battery voltages
          for (uint8_t i = 0; i < batteryCount; ++i)
            lampda_print("cell %d: %d mV, is balancing: %s",
                         i,
                         balancerStatus.batteryVoltages_mV[i],
                         boolToString(balancerStatus.isBalancing[i]));
          lampda_print("total (from balancer) %dmv\n", balancerStatus.stackVoltage_mV);
        }
        else
        {
          lampda_print("balancer measurments not valid");
        }

        const auto& chargerStatus = charger::get_state();
        const bool areChargerValueValid = chargerStatus.areMeasuresOk;
        if (areChargerValueValid)
        {
          lampda_print("total (from charger) %dmv", chargerStatus.batteryVoltage_mV);
        }
        else
        {
          lampda_print("charger measurments not valid");
        }

        if (areChargerValueValid or areBalancerValueValid)
        {
          // print individual battery voltages
          lampda_print(
                  "raw battery level:%.2f%%\n"
                  "battery level:%.2f%%\n"
                  "minimum cell level:%.2f%%",
                  battery::get_level_percent(battery::get_raw_battery_voltage_mv()) / 100.0,
                  battery::get_battery_level() / 100.0,
                  battery::get_battery_minimum_cell_level() / 100.0);
        }
        else
        {
          lampda_print("Battery measurments not valid");
        }
        break;
      }

    case utils::hash("cinfo"):
      {
        const auto& chargerState = charger::get_state();

        lampda_print(
                "is charge signal ok:%s\n"
                "voltage on vbus:%dmV\n"
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
                boolToString(charger::is_vbus_signal_detected()),
                boolToString(chargerState.is_charging()),
                boolToString(chargerState.is_effectivly_charging()),
                battery::get_battery_level() / 100.0,
                chargerState.get_status_str().c_str());
        // in case there is a software error, display it
        if (chargerState.status == charger::Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          lampda_print("\t hardware error detail: \"%s\"", chargerState.hardwareErrorMessage.c_str());
        }
        if (chargerState.status == charger::Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          lampda_print("\t software error detail: \"%s\"", chargerState.softwareErrorMessage.c_str());
        }
        break;
      }

    case utils::hash("alerts"):
      alerts::show_all();
      break;

    case utils::hash("i2c"):
      {
        lampda_print(
                "fusb detected : %d\n"
                "imu detected: %d\n"
                "balancer detected: %d\n"
                "charger detected: %d",
                i2c_check_existence(0, pdNegociationI2cAddress) == 0,
                i2c_check_existence(0, imuI2cAddress) == 0,
                i2c_check_existence(0, batteryBalancerI2cAddress) == 0,
                i2c_check_existence(0, chargeI2cAddress) == 0);
        break;
      }

    case utils::hash("ADC"):
      {
        const auto& chargerState = charger::get_state();
        lampda_print(
                "PowerRail voltage:%dmV\n"
                "PowerRail current:%dmA\n"
                "VBUS voltage:%dmA\n"
                "Bat voltage:%dmV\n"
                "Bat current:%dmA\n"
                "Temperature:%.2fC",
                chargerState.powerRail_mV,
                chargerState.inputCurrent_mA,
                powerDelivery::get_vbus_voltage(),
                chargerState.batteryVoltage_mV,
                chargerState.batteryCurrent_mA,
                read_CPU_temperature_degreesC());
        break;
      }

    case utils::hash("PD"):
      {
        powerDelivery::show_pd_status();
        const auto& pd = powerDelivery::get_available_pd();
        if (pd.empty())
        {
          lampda_print("No power delivery capabilities");
        }
        else
        {
          lampda_print("Power delivery profiles :");
          for (const auto& pdo: pd)
            lampda_print("- %dmV, %dmA", pdo.voltage_mv, pdo.maxCurrent_mA);
        }
        break;
      }

    case utils::hash("states"):
      {
        lampda_print("behavior machine state:%s. error msgs: %s",
                     behavior::get_state().c_str(),
                     behavior::get_error_state_message().c_str());
        lampda_print("power state machine state: %s. error msgs: %s",
                     power::get_state().c_str(),
                     power::get_error_string().c_str());
        break;
      }

    case utils::hash("format-fs"):
      lampda_print("clearing the whole file format");
      fileSystem::clear_internal_fs();
      break;

    case utils::hash("DFU"):
      enter_serial_dfu();
      break;

    case utils::hash("tasks"):
      char buff[512];
      get_thread_debug(buff);
      lampda_print("%s", buff);
      break;

    default:
      lampda_print("unknown command: \'%s\'", command.c_str());
      lampda_print("type h for available commands");
      break;
  }
}

void setup() { init_prints(); }

void handleSerialEvents()
{
  const auto& inputs = read_inputs();
  for (const std::string& input: inputs)
  {
    handleCommand(input);
  }
}

} // namespace serial
