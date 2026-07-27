#include "command_line_interface.h"

#include "src/system/logic/behavior.h"

#include "src/system/power/charger.h"
#include "src/system/power/balancer.h"
#include "src/system/power/PDlib/power_delivery.h"

#include "src/system/platform/bluetooth.h"
#include "src/system/platform/i2c.h"
#include "src/system/platform/print.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/threads.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/button.h"
#include "src/system/physical/fileSystem.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/inputs_bluetooth.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/statistics_handler.h"

#include <cstdlib>
#include <cerrno>
#include <climits>

namespace lampda {
namespace logic {
namespace cli {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;
constexpr uint8_t maxArgumentCount = 8;

inline const char* boolToString(bool b) { return b ? "true" : "false"; }

inline bool isCommandSeparator(const char character) { return character == ' ' || character == '\t'; }

struct ParsedCommand
{
  platform::Inputs::Command buffer{};

  uint8_t commandOffset = 0;
  std::array<uint8_t, maxArgumentCount> argumentOffsets{};
  uint8_t argumentCount = 0;

  const char* name() const
  {
    return buffer.data() + commandOffset;
  }

  const char* argument(const size_t index) const
  {
    if (index >= argumentCount)
      return nullptr;

    return buffer.data() + argumentOffsets[index];
  }
};


bool parse_uint8_argument(
        const ParsedCommand& command,
        const size_t index,
        uint8_t& value)
{
  const char* text = command.argument(index);

  if (text == nullptr || *text == '\0')
    return false;

  char* end = nullptr;
  errno = 0;

  const unsigned long tmp_value = std::strtoul(text, &end, 0);

  if (errno != 0 || end == text || *end != '\0' || tmp_value > UINT8_MAX)
    return false;

  value = static_cast<uint8_t>(tmp_value);
  return true;
}

bool parse_uint16_argument(
        const ParsedCommand& command,
        const size_t index,
        uint16_t& value)
{
  const char* text = command.argument(index);

  if (text == nullptr)
    return false;

  char* end = nullptr;
  errno = 0;

  const unsigned long tmp_value = std::strtoul(text, &end, 0);

  if (errno != 0 ||
      end == text ||
      *end != '\0'|| 
      tmp_value > UINT16_MAX)
  {
    return false;
  }

  value = tmp_value;
  return true;
}

ParsedCommand parseCommand(const platform::Inputs::Command& input)
{
  ParsedCommand result;
  // copy str
  result.buffer = input;

  size_t position = 0;

  // skip all first blank char define in isCommandSeparator
  while (position < result.buffer.size() &&
         isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  result.commandOffset = static_cast<uint8_t>(position);

  // looking for the end of the command
  while (position < result.buffer.size() &&
         result.buffer[position] != '\0' &&
         !isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  // put a \0 at the end (useful for the hash command and some print)
  if (position < result.buffer.size() &&
      result.buffer[position] != '\0')
  {
    result.buffer[position] = '\0';
    ++position;
  }

  // now, parse every argument
  while (position < result.buffer.size() &&
         result.argumentCount < maxArgumentCount)
  {
    // while it's a blank char
    while (position < result.buffer.size() &&
           isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // terminated condition of the while (end of the string)
    if (position >= result.buffer.size() ||
        result.buffer[position] == '\0')
    {
      break;
    }

    //save the offset
    result.argumentOffsets[result.argumentCount++] = static_cast<uint8_t>(position);

    // looking for the end of the argument 
    while (position < result.buffer.size() &&
           result.buffer[position] != '\0' &&
           !isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // put a \0 at the end
    if (position < result.buffer.size() &&
        result.buffer[position] != '\0')
    {
      result.buffer[position] = '\0';
      ++position;
    }
  }

  return result;
}



void handleCommand(const platform::Inputs::Command& commandLine)
{
  const ParsedCommand command = parseCommand(commandLine);

  if (command.name()[0] == '\0')
    return;

  switch (utils::hash(command.name()))
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
                "ble: debug bluetooth informations\n"
                "echo <args>{0-8}: display parsed arguments\n"
                "brightness <[0-1024]>: update the brightness \n"
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
        const auto& balancerStatus = ::lampda::power::balancer::get_status();
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

        const auto& chargerStatus = ::lampda::power::charger::get_state();
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
        const auto& chargerState = ::lampda::power::charger::get_state();
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
                  "was battery recover: %d\n"
                  "battery level:%.2f%%\n"
                  "-> charger status: %s",
                  boolToString(chargerState.isChargeOkSignalHigh),
                  chargerState.powerRail_mV,
                  chargerState.inputCurrent_mA,
                  chargerState.batteryVoltage_mV,
                  chargerState.chargeCurrent_mA,
                  boolToString(::lampda::power::charger::is_vbus_signal_detected()),
                  boolToString(chargerState.is_charging()),
                  boolToString(chargerState.is_effectivly_charging()),
                  ::lampda::logic::power::was_started_in_battery_recovery(),
                  physical::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }
        else
        {
          platform::lampda_print(
                  "is charge signal ok:%s\n"
                  "Charger measurments are invalid !!\n"
                  "is usb serial connected:%s\n"
                  "was battery recover: %d\n"
                  "battery level:%.2f%%\n"
                  "-> charger status: %s",
                  boolToString(chargerState.isChargeOkSignalHigh),
                  boolToString(::lampda::power::charger::is_vbus_signal_detected()),
                  ::lampda::logic::power::was_started_in_battery_recovery(),
                  physical::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }

        // in case there is a software error, display it
        if (chargerState.status == ::lampda::power::charger::Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          platform::lampda_print("\t hardware error detail: \"%s\"", chargerState.hardwareErrorMessage.c_str());
        }
        if (chargerState.status == ::lampda::power::charger::Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
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
        const auto& chargerState = ::lampda::power::charger::get_state();
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
                  ::lampda::power::powerDelivery::get_vbus_voltage(),
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
                  ::lampda::power::powerDelivery::get_vbus_voltage(),
                  platform::registers::read_CPU_temperature_degreesC());
        }
        break;
      }

    case utils::hash("PD"):
      {
        ::lampda::power::powerDelivery::show_pd_status();
        const auto& pd = ::lampda::power::powerDelivery::get_available_pd();
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
                               logic::power::get_state().c_str(),
                               logic::power::get_error_string().c_str());
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
        switch (physical::button::get_button_pin())
        {
          case platform::gpio::DigitalPin::GPIO::gpio3:
            physical::button::set_button_pin(platform::gpio::DigitalPin::GPIO::gpio4);
            platform::lampda_print("Set button pin to gpio4");
            break;
          case platform::gpio::DigitalPin::GPIO::gpio4:
#ifdef LMBD_LAMP_TYPE__SIMPLE
            physical::button::set_button_pin(platform::gpio::DigitalPin::GPIO::gpio6);
            platform::lampda_print("Set button pin to gpio6");
            break;
            // The simple lamp can also use pin 6
          case platform::gpio::DigitalPin::GPIO::gpio6: // pass throught
#endif
          default:
            physical::button::set_button_pin(platform::gpio::DigitalPin::GPIO::gpio3);
            platform::lampda_print("Set button pin to gpio3");
            break;
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

    case utils::hash("ble"):
      platform::lampda_print(
              "is activated: %d\n"
              "is advertising: %d\n"
              "is connected: %d\n"
              "is msg received: %d\n"
              "auto activations left: %d",
              platform::bluetooth::is_activated(),
              platform::bluetooth::is_advertising(),
              platform::bluetooth::is_connected(),
              logic::inputs_bluetooth::is_bluetooth_used(),
              logic::behavior::internal::get_bluetooth_auto_activation_left());
      break;
    case utils::hash("echo"):
      platform::lampda_print(
              "command: %s, argument count: %u",
              command.name(),
              command.argumentCount);

      for (uint8_t index = 0;
          index < command.argumentCount;
          ++index)
      {
        platform::lampda_print(
                "argument %u: '%s'",
                index,
                command.argument(index));
      }
      break;
    case utils::hash("brightness"):
    {
      brightness_t brightness = 0;

      if (parse_uint16_argument(command, 0, brightness) && brightness <= lampda::logic::brightness::get_max_brightness() )
      {
        platform::lampda_print("brightness: %u", brightness);
        lampda::logic::brightness::update_brightness(brightness, false);
        break; 
      }
      platform::lampda_print("usage: brightness <0-1024>");
      break;
    }      
    default:
      platform::lampda_print("unknown command: \'%s\'", command.name());
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

} // namespace cli
} // namespace logic
} // namespace lampda
