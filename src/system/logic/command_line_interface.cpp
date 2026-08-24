#include "command_line_interface.h"

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/i2c.h"
#include "src/system/hal/registers.h"
#include "src/system/hal/serial.h"
#include "src/system/hal/time.h"

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

namespace user_commands {

void help_hook();
static void cmd_help(const ParsedCommand&) { help_hook(); }

static void cmd_summary(const ParsedCommand&)
{
  bsp::lampda_print("--- System Summary ---");
  // version
  bsp::lampda_print("v%d.%d", USER_SOFTWARE_VERSION_MAJOR, USER_SOFTWARE_VERSION_MINOR);
  bsp::lampda_print("Logic state: %s", logic::behavior::get_state().c_str());
  // Battery
  auto batt = component::battery::get_battery_level() / 100.0;
  bsp::lampda_print("Battery: %.1f%%", batt);
  // Power
  auto charger = ::lampda::component::charger::get_state();
  if (charger.areMeasuresOk)
  {
    bsp::lampda_print("Charger: %s | prail: %dmV @ %dmA",
                      charger.is_charging() ? "Charging" : "Not Charging",
                      charger.powerRail_mV,
                      charger.inputCurrent_mA);
  }
  else
    bsp::lampda_print("Charger: offline");
  // Bluetooth
  bsp::lampda_print("Bluetooth: %s", hal::bluetooth::is_connected() ? "Connected" : "Disconnected");
  // Temperature
  bsp::lampda_print("Temp: %.1f°C", hal::registers::read_CPU_temperature_degreesC());
  // serial number
  bsp::lampda_print("SN: %lu", hal::registers::get_device_serial_number());
  bsp::lampda_print("--------------------");
}

static void cmd_version(const ParsedCommand&)
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
}

static void cmd_type(const ParsedCommand&)
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
}

static void cmd_id(const ParsedCommand&) { bsp::lampda_print("SN: %lu", hal::registers::get_device_serial_number()); }

static void cmd_stats(const ParsedCommand&) { logic::statistics::show(); }

static void cmd_bat(const ParsedCommand&)
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
}

static void cmd_cinfo(const ParsedCommand&)
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
}

static void cmd_adc(const ParsedCommand&)
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
}

static void cmd_powerdelivery(const ParsedCommand&)
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
}

static void cmd_states(const ParsedCommand&)
{
  bsp::lampda_print("behavior machine state:%s. error msgs: %s",
                    logic::behavior::get_state().c_str(),
                    logic::behavior::get_error_state_message().c_str());
  bsp::lampda_print("power state machine state: %s. error msgs: %s",
                    logic::power::get_state().c_str(),
                    logic::power::get_error_string().c_str());
}

static void cmd_alerts(const ParsedCommand&) { logic::alerts::show_all(); }

static void cmd_i2c(const ParsedCommand&)
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
}

static void cmd_format(const ParsedCommand&)
{
  bsp::lampda_print("clearing the whole file format");
  component::fileSystem::clear_internal_fs();
}

static void cmd_dfu(const ParsedCommand&) { hal::registers::enter_serial_dfu(); }

static void cmd_buttontoggle(const ParsedCommand&)
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
}

static void cmd_shutdown(const ParsedCommand&) { logic::behavior::internal::handle_shutdown_state(); }

static void cmd_tasks(const ParsedCommand&)
{
  char buff[512];
  bsp::threads::get_thread_debug(buff);
  bsp::lampda_print("%s", buff);
}

static void cmd_ble(const ParsedCommand&)
{
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
}

static void cmd_echo(const ParsedCommand& command)
{
  bsp::lampda_print("command: %s, argument count: %u", command.name(), command.argumentCount);

  for (uint8_t index = 0; index < command.argumentCount; ++index)
  {
    bsp::lampda_print("argument %u: '%s'", index, command.argument(index));
  }
}

static void cmd_brightness(const ParsedCommand& command)
{
  brightness_t brightness = 0;
  if (argument::parse_uint16(command, 0, brightness) && brightness <= logic::brightness::get_max_brightness())
  {
    bsp::lampda_print("set brightness to %u/%u", brightness, logic::brightness::get_max_brightness());
    logic::brightness::update_brightness(brightness, false);
    return;
  }
  bsp::lampda_print("usage: brightness <0-%u>", logic::brightness::get_max_brightness());
}

static void cmd_time(const ParsedCommand&)
{
  const auto& time = component::time::get_real_time();
  if (time.is_valid())
  {
    bsp::lampda_print(
            "Real time is %dh %dmin %ds, day index is %d", time.hour, time.minutes, time.seconds, time.dayOfTheWeek);
  }
  else
  {
    bsp::lampda_print("Real time is not set");
  }
}

static void cmd_serial(const ParsedCommand&)
{
  bsp::lampda_print("Local serial port: active %d, ", hal::serial::is_activated());
  bsp::lampda_print("BLE serial port: active %d, ", hal::bluetooth::serial::is_activated());
}

struct Command
{
  const char* const name;                ///< Name of the command to call
  const char* const usage;               ///< Command usage string (e.g., "[arg1] arg2")
  uint8_t minArgs;                       ///< minimum argument count
  uint8_t maxArgs;                       ///< maximum argument count
  const char* const description;         ///< Text description of the command
  void (*handler)(const ParsedCommand&); ///< handler of the command
  const uint32_t hash;                   ///< command name hash
};
constexpr Command make_command(const char* name, const char* desc, void (*handler)(const ParsedCommand&))
{
  return {name, nullptr, 0, 0, desc, handler, utils::hash(name)};
}
constexpr Command make_command_args(const char* name,
                                    const char* usage,
                                    const uint8_t minArgs,
                                    const uint8_t maxArgs,
                                    const char* desc,
                                    void (*handler)(const ParsedCommand&))
{
  return {name, usage, minArgs, maxArgs, desc, handler, utils::hash(name)};
}

/// Define the CLI commands here
constexpr Command commands[] = {
        make_command("h", "this page", cmd_help),
        make_command("sum", "system summary", cmd_summary),
        make_command("v", "hardware & software version", cmd_version),
        make_command("t", "return the lamp type", cmd_type),
        make_command("id", "return the board serial number", cmd_id),
        make_command("stats", "display the system use statistics", cmd_stats),
        make_command("bat", " battery info/levels", cmd_bat),
        make_command("cinfo", "charger infos", cmd_cinfo),
        make_command("adc", "values from the charger ADC", cmd_adc),
        make_command("pd", "display the connected PD capabilities", cmd_powerdelivery),
        make_command("states", "state machine states", cmd_states),
        make_command("alerts", "show all raised alerts", cmd_alerts),
        make_command("i2c", "start an i2c present check", cmd_i2c),
        make_command("format", "format the whole file system (dangerous)", cmd_format),
        make_command("dfu", "clear this program from memory, enter update mode", cmd_dfu),
        make_command("buttonTogg", "change the button pin number for the next boot", cmd_buttontoggle),
        make_command("shutdown", "force shutdown the lamp", cmd_shutdown),
        make_command("tasks", "display a debug of task usages", cmd_tasks),
        make_command("ble", "debug bluetooth informations", cmd_ble),
        make_command("time", "show current time", cmd_time),
        make_command("serial", "show serial infos", cmd_serial),
        make_command_args("echo", "[<arg>...]", 0, maxArgumentCount, "display parsed arguments", cmd_echo),
        make_command_args("brgt", "[0-1024]", 1, 1, "Set the output brightness", cmd_brightness),
};

constexpr bool has_duplicate_hash(const Command* arr, size_t count)
{
#ifdef LMBD_CPP17
  for (size_t i = 0; i < count; ++i)
  {
    for (size_t j = i + 1; j < count; ++j)
    {
      if (arr[i].hash == arr[j].hash)
      {
        return true;
      }
    }
  }
#endif
  return false;
}
/// Check for command uniqueness
static_assert(!has_duplicate_hash(commands, sizeof(commands) / sizeof(commands[0])),
              "Duplicate command hash detected! Ensure all command names are unique.");

/// hook that can read the other commands and print them out
void help_hook()
{
  bsp::lampda_print("---Lamp-da CLI---");
  for (const auto& cmd: commands)
  {
    if (cmd.usage)
      bsp::lampda_print("%s %s: %s", cmd.name, cmd.usage, cmd.description);
    else
      bsp::lampda_print("%s: %s", cmd.name, cmd.description);
    // let the thread rest, this command list can be big
    hal::delay_ms(5);
  }
  bsp::lampda_print("-----------------");
}

} // namespace user_commands

void handleCommand(const bsp::text_in::Inputs::Command& commandLine)
{
  const ParsedCommand command = parseCommand(commandLine);

  if (command.name()[0] == '\0')
    return;

  const auto cmdHash = utils::hash(command.name());
  for (const auto& cmd: user_commands::commands)
  {
    if (cmd.hash == cmdHash)
    {
      // check arguments
      if (command.argumentCount < cmd.minArgs or command.argumentCount > cmd.maxArgs)
      {
        if (cmd.minArgs == cmd.maxArgs)
          bsp::lampda_print("Command \"%s\" usage must have %d arguments. Type h for usage.", cmd.name, cmd.minArgs);
        else
          bsp::lampda_print("Command \"%s\" usage must have between %d and %d arguments. Type h for usage.",
                            cmd.name,
                            cmd.minArgs,
                            cmd.maxArgs);
        return;
      }

      cmd.handler(command);
      return;
    }
  }

  bsp::lampda_print("unknown command: \'%s\'", command.name());
  bsp::lampda_print("type h for available commands");
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
}

} // namespace cli
} // namespace logic
} // namespace lampda
