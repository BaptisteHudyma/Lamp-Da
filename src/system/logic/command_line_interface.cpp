#include "command_line_interface.h"

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/i2c.h"
#include "src/system/hal/registers.h"

#include "src/system/bsp/pd/power_delivery.h"
#include "src/system/bsp/balancer.h"
#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/component/battery.h"
#include "src/system/component/button.h"
#include "src/system/component/charger.h"
#include "src/system/component/fileSystem.h"
#include "src/system/component/time_handling.h"

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
  bsp::text_in::Inputs::Command buffer {};

  uint8_t commandOffset = 0;
  std::array<uint8_t, maxArgumentCount> argumentOffsets {};
  uint8_t argumentCount = 0;

  const char* name() const { return buffer.data() + commandOffset; }

  const char* argument(const size_t index) const
  {
    if (index >= argumentCount)
      return nullptr;

    return buffer.data() + argumentOffsets[index];
  }
};

/// Tools to parse text arguments
namespace argument {

/// Generic parse function for unsigned integers
bool parse_uint(const ParsedCommand& command, const size_t index, unsigned long& value)
{
  const char* text = command.argument(index);

  if (text == nullptr)
    return false;

  char* end = nullptr;
  errno = 0;

  const unsigned long tmp_value = std::strtoul(text, &end, 0);

  if (errno != 0 || end == text || *end != '\0')
  {
    return false;
  }
  value = tmp_value;
  return true;
}

/// Parse an argument as a 8 bit unsigned number
bool parse_uint8(const ParsedCommand& command, const size_t index, uint8_t& value)
{
  unsigned long argument;
  const bool isValid = parse_uint(command, index, argument);
  if (!isValid)
    return false;
  if (argument > UINT8_MAX)
    return false;

  value = static_cast<uint8_t>(argument);
  return true;
}

/// Parse an argument as a 16 bit unsigned number
bool parse_uint16(const ParsedCommand& command, const size_t index, uint16_t& value)
{
  unsigned long argument;
  const bool isValid = parse_uint(command, index, argument);
  if (!isValid)
    return false;
  if (argument > UINT16_MAX)
    return false;

  value = argument;
  return true;
}

} // namespace argument

ParsedCommand parseCommand(const bsp::text_in::Inputs::Command& input)
{
  ParsedCommand result;
  // copy str
  result.buffer = input;

  size_t position = 0;

  // skip all first blank char define in isCommandSeparator
  while (position < result.buffer.size() && isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  result.commandOffset = static_cast<uint8_t>(position);

  // looking for the end of the command
  while (position < result.buffer.size() && result.buffer[position] != '\0' &&
         !isCommandSeparator(result.buffer[position]))
  {
    ++position;
  }

  // put a \0 at the end (useful for the hash command and some print)
  if (position < result.buffer.size() && result.buffer[position] != '\0')
  {
    result.buffer[position] = '\0';
    ++position;
  }

  // now, parse every argument
  while (position < result.buffer.size() && result.argumentCount < maxArgumentCount)
  {
    // while it's a blank char
    while (position < result.buffer.size() && isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // terminated condition of the while (end of the string)
    if (position >= result.buffer.size() || result.buffer[position] == '\0')
    {
      break;
    }

    // save the offset
    result.argumentOffsets[result.argumentCount++] = static_cast<uint8_t>(position);

    // looking for the end of the argument
    while (position < result.buffer.size() && result.buffer[position] != '\0' &&
           !isCommandSeparator(result.buffer[position]))
    {
      ++position;
    }

    // put a \0 at the end
    if (position < result.buffer.size() && result.buffer[position] != '\0')
    {
      result.buffer[position] = '\0';
      ++position;
    }
  }

  return result;
}

void handleCommand(const bsp::text_in::Inputs::Command& commandLine)
{
  const ParsedCommand command = parseCommand(commandLine);

  if (command.name()[0] == '\0')
    return;

  switch (utils::hash(command.name()))
  {
    case utils::hash("h"):
    case utils::hash("help"):
      {
        bsp::lampda_print(
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
                "brightness <[0-1024]>: update the brightness\n"
                "time: show current time\n"
                "-----------------");
        break;
      }

    case utils::hash("v"):
      {
        bsp::lampda_print(
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
        bsp::lampda_print("indexable");
#else
#ifdef LMBD_LAMP_TYPE__SIMPLE
        bsp::lampda_print("simple");
#else
#ifdef LMBD_LAMP_TYPE__CCT
        bsp::lampda_print("cct");
#else
#error "Unspecified lamp type in CLI"
#endif /* LMBD_LAMP_TYPE__CCT */
#endif /* LMBD_LAMP_TYPE__SIMPLE */
#endif /* LMBD_LAMP_TYPE__INDEXABLE */
        break;
      }

    case utils::hash("id"):
      {
        bsp::lampda_print("Serial number: %lu", hal::registers::get_device_serial_number());
        break;
      }

    case utils::hash("stats"):
      {
        logic::statistics::show();
        break;
      }

    case utils::hash("bat"):
      {
        const auto& balancerStatus = ::lampda::bsp::balancer::get_status();
        const bool areBalancerValueValid = balancerStatus.is_valid();

        if (areBalancerValueValid)
        {
          // print individual battery voltages
          for (uint8_t i = 0; i < batteryCount; ++i)
            bsp::lampda_print("cell %d: %d mV, is balancing: %s",
                              i,
                              balancerStatus.batteryVoltages_mV[i],
                              boolToString(balancerStatus.isBalancing[i]));
          bsp::lampda_print("total (from balancer) %dmv\n", balancerStatus.stackVoltage_mV);
        }
        else
        {
          bsp::lampda_print("balancer measurments not valid");
        }

        const auto& chargerStatus = ::lampda::component::charger::get_state();
        const bool areChargerValueValid = chargerStatus.areMeasuresOk;
        if (areChargerValueValid)
        {
          bsp::lampda_print("total (from charger) %dmv", chargerStatus.batteryVoltage_mV);
        }
        else
        {
          bsp::lampda_print("charger measurments not valid");
        }

        if (areChargerValueValid or areBalancerValueValid)
        {
          // print individual battery voltages
          bsp::lampda_print(
                  "raw battery level:%.2f%%\n"
                  "battery level:%.2f%%\n"
                  "minimum cell level:%.2f%%",
                  component::battery::get_level_percent(component::battery::get_raw_battery_voltage_mv()) / 100.0,
                  component::battery::get_battery_level() / 100.0,
                  component::battery::get_battery_minimum_cell_level() / 100.0);
        }
        else
        {
          bsp::lampda_print("Battery measurments not valid");
        }
        break;
      }

    case utils::hash("cinfo"):
      {
        const auto& chargerState = ::lampda::component::charger::get_state();
        if (chargerState.areMeasuresOk)
        {
          bsp::lampda_print(
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
                  boolToString(::lampda::component::charger::is_vbus_signal_detected()),
                  boolToString(chargerState.is_charging()),
                  boolToString(chargerState.is_effectivly_charging()),
                  ::lampda::logic::power::was_started_in_battery_recovery(),
                  component::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }
        else
        {
          bsp::lampda_print(
                  "is charge signal ok:%s\n"
                  "Charger measurments are invalid !!\n"
                  "is usb serial connected:%s\n"
                  "was battery recover: %d\n"
                  "battery level:%.2f%%\n"
                  "-> charger status: %s",
                  boolToString(chargerState.isChargeOkSignalHigh),
                  boolToString(::lampda::component::charger::is_vbus_signal_detected()),
                  ::lampda::logic::power::was_started_in_battery_recovery(),
                  component::battery::get_battery_level() / 100.0,
                  chargerState.get_status_str().c_str());
        }

        // in case there is a software error, display it
        if (chargerState.status == ::lampda::component::charger::Charger_t::ChargerStatus_t::ERROR_HARDWARE)
        {
          bsp::lampda_print("\t hardware error detail: \"%s\"", chargerState.hardwareErrorMessage.c_str());
        }
        if (chargerState.status == ::lampda::component::charger::Charger_t::ChargerStatus_t::ERROR_SOFTWARE)
        {
          bsp::lampda_print("\t software error detail: \"%s\"", chargerState.softwareErrorMessage.c_str());
        }
        break;
      }

    case utils::hash("alerts"):
      logic::alerts::show_all();
      break;

    case utils::hash("i2c"):
      {
        bsp::lampda_print(
                "fusb detected : %d\n"
                "imu detected: %d\n"
                "balancer detected: %d\n"
                "charger detected: %d",
                hal::i2c::i2c_check_existence(0, hal::i2c::pdNegociationI2cAddress) == 0,
                hal::i2c::i2c_check_existence(0, hal::i2c::imuI2cAddress) == 0,
                hal::i2c::i2c_check_existence(0, hal::i2c::batteryBalancerI2cAddress) == 0,
                hal::i2c::i2c_check_existence(0, hal::i2c::chargeI2cAddress) == 0);
        break;
      }

    case utils::hash("ADC"):
      {
        const auto& chargerState = ::lampda::component::charger::get_state();
        if (chargerState.areMeasuresOk)
        {
          bsp::lampda_print(
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
                  ::lampda::bsp::powerDelivery::get_vbus_voltage(),
                  chargerState.batteryVoltage_mV,
                  chargerState.batteryCurrent_mA,
                  hal::registers::read_CPU_temperature_degreesC());
        }
        else
        {
          bsp::lampda_print(
                  "Charger measurment are invalid !\n"
                  "Last update %dms\n"
                  "VBUS voltage:%dmA\n"
                  "Temperature:%.2fC",
                  chargerState.lastUpdateTime_ms,
                  ::lampda::bsp::powerDelivery::get_vbus_voltage(),
                  hal::registers::read_CPU_temperature_degreesC());
        }
        break;
      }

    case utils::hash("PD"):
      {
        ::lampda::bsp::powerDelivery::show_pd_status();
        const auto& pd = ::lampda::bsp::powerDelivery::get_available_pd();
        if (pd.empty())
        {
          bsp::lampda_print("No power delivery capabilities");
        }
        else
        {
          bsp::lampda_print("Power delivery profiles :");
          for (const auto& pdo: pd)
            bsp::lampda_print("- %dmV, %dmA", pdo.voltage_mv, pdo.maxCurrent_mA);
        }
        break;
      }

    case utils::hash("states"):
      {
        bsp::lampda_print("behavior machine state:%s. error msgs: %s",
                          logic::behavior::get_state().c_str(),
                          logic::behavior::get_error_state_message().c_str());
        bsp::lampda_print("power state machine state: %s. error msgs: %s",
                          logic::power::get_state().c_str(),
                          logic::power::get_error_string().c_str());
        break;
      }

    case utils::hash("format-fs"):
      bsp::lampda_print("clearing the whole file format");
      component::fileSystem::clear_internal_fs();
      break;

    case utils::hash("DFU"):
      hal::registers::enter_serial_dfu();
      break;

    case utils::hash("buttonTogg"):
      {
        switch (component::button::get_button_pin())
        {
          case hal::gpio::DigitalPin::GPIO::gpio3:
            component::button::set_button_pin(hal::gpio::DigitalPin::GPIO::gpio4);
            bsp::lampda_print("Set button pin to gpio4");
            break;
          case hal::gpio::DigitalPin::GPIO::gpio4:
#ifdef LMBD_LAMP_TYPE__SIMPLE
            component::button::set_button_pin(hal::gpio::DigitalPin::GPIO::gpio6);
            bsp::lampda_print("Set button pin to gpio6");
            break;
            // The simple lamp can also use pin 6
          case hal::gpio::DigitalPin::GPIO::gpio6: // pass throught
#endif
          default:
            component::button::set_button_pin(hal::gpio::DigitalPin::GPIO::gpio3);
            bsp::lampda_print("Set button pin to gpio3");
            break;
        }
        break;
      }

    case utils::hash("shutdown"):
      logic::behavior::internal::handle_shutdown_state();
      break;

    case utils::hash("tasks"):
      char buff[512];
      bsp::threads::get_thread_debug(buff);
      bsp::lampda_print("%s", buff);
      break;

    case utils::hash("ble"):
      bsp::lampda_print(
              "is activated: %d\n"
              "is advertising: %d\n"
              "is connected: %d\n"
              "is msg received: %d\n"
              "auto activations left: %d",
              hal::bluetooth::is_activated(),
              hal::bluetooth::is_advertising(),
              hal::bluetooth::is_connected(),
              logic::inputs_bluetooth::is_bluetooth_used(),
              logic::behavior::internal::get_bluetooth_auto_activation_left());
      break;
    case utils::hash("echo"):
      bsp::lampda_print("command: %s, argument count: %u", command.name(), command.argumentCount);

      for (uint8_t index = 0; index < command.argumentCount; ++index)
      {
        bsp::lampda_print("argument %u: '%s'", index, command.argument(index));
      }
      break;
    case utils::hash("brightness"):
      {
        brightness_t brightness = 0;
        if (argument::parse_uint16(command, 0, brightness) && brightness <= logic::brightness::get_max_brightness())
        {
          bsp::lampda_print("set brightness to %u/%u", brightness, logic::brightness::get_max_brightness());
          logic::brightness::update_brightness(brightness, false);
          break;
        }
        bsp::lampda_print("usage: brightness <0-%u>", logic::brightness::get_max_brightness());
        break;
      }
    case utils::hash("time"):
      {
        const auto& time = component::time::get_real_time();
        if (time.is_valid())
        {
          bsp::lampda_print("Real time is %dh %dmin %ds, day index is %d",
                            time.hour,
                            time.minutes,
                            time.seconds,
                            time.dayOfTheWeek);
        }
        else
        {
          bsp::lampda_print("Real time is not set");
        }
        break;
      }

    default:
      bsp::lampda_print("unknown command: \'%s\'", command.name());
      bsp::lampda_print("type h for available commands");
      break;
  }
}

void setup() { bsp::lampda_print_init(); }

void handleSerialEvents()
{
  const auto& inputs = bsp::text_in::read_inputs();
  for (size_t i = 0; i < min<uint8_t>(bsp::text_in::Inputs::maxCommands, inputs.commandCount); i++)
  {
    const bsp::text_in::Inputs::Command& input = inputs.commandList[i];
    handleCommand(input);
  }

  const auto& inputsBLE = hal::bluetooth::read_uart();
  for (size_t i = 0; i < min<uint8_t>(bsp::text_in::Inputs::maxCommands, inputsBLE.commandCount); i++)
  {
    const bsp::text_in::Inputs::Command& input = inputsBLE.commandList[i];
    handleCommand(input);
  }
}

} // namespace cli
} // namespace logic
} // namespace lampda
