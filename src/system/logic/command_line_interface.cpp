#include "command_line_interface.h"

#include "src/system/hal/i2c.h"
#include "src/system/hal/registers.h"
#include "src/system/hal/serial.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/pd/power_delivery.h"
#include "src/system/bsp/balancer.h"
#include "src/system/bsp/ble.h"
#include "src/system/bsp/filesystem.h"
#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/component/battery.h"
#include "src/system/component/button.h"
#include "src/system/component/charger.h"
#include "src/system/component/sound.h"
#include "src/system/component/time_handling.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/common/cmd_parser.h"
#include "src/system/common/user_commands.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/battery_blink.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/button_bindings.h"
#include "src/system/logic/inputs_bluetooth.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/statistics_handler.h"
#include "src/system/logic/sun_timers.h"

#include "src/user/functions.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace lampda {
namespace logic {
namespace cli {

struct Command
{
  const char* const name;                             ///< Name of the command to call
  const char* const usage;                            ///< Command usage string (e.g., "[arg1] arg2")
  uint8_t minArgs;                                    ///< minimum argument count
  uint8_t maxArgs;                                    ///< maximum argument count
  const char* const description;                      ///< Text description of the command
  void (*handler)(const common::cli::ParsedCommand&); ///< handler of the command
  const uint32_t hash;                                ///< command name hash
};
constexpr Command make_command(const char* name, const char* desc, void (*handler)(const common::cli::ParsedCommand&))
{
  return {name, nullptr, 0, 0, desc, handler, utils::hash(name)};
}
constexpr Command make_command_args(const char* name,
                                    const char* usage,
                                    const uint8_t minArgs,
                                    const uint8_t maxArgs,
                                    const char* desc,
                                    void (*handler)(const common::cli::ParsedCommand&))
{
  return {name, usage, minArgs, maxArgs, desc, handler, utils::hash(name)};
}

inline const char* boolToString(bool b) { return b ? "true" : "false"; }

/// Display the usage and description of a command, with \p parent the name of its command group, if any
static void print_command_description(const Command& cmd, const char* parent)
{
  if (parent)
  {
    if (cmd.usage)
      bsp::lampda_print("%s %s %s: %s", parent, cmd.name, cmd.usage, cmd.description);
    else
      bsp::lampda_print("%s %s: %s", parent, cmd.name, cmd.description);
  }
  else
  {
    if (cmd.usage)
      bsp::lampda_print("%s %s: %s", cmd.name, cmd.usage, cmd.description);
    else
      bsp::lampda_print("%s: %s", cmd.name, cmd.description);
  }
}

/// Parse a command and handle its effects
template<size_t N> void handleCommand(const common::cli::ParsedCommand& command,
                                      const Command (&commandsToParse)[N],
                                      const char* calledCommand = nullptr)
{
  if (command.name()[0] == '\0')
    return;

  const auto cmdHash = utils::hash(command.name());
  for (const auto& cmd: commandsToParse)
  {
    if (cmd.hash == cmdHash)
    {
      // "<command> h": display the command description, before the arguments check.
      // Commands taking any argument count forward them, and handle "h" themselves (set h, sys h, act h).
      const bool forwardsArguments = cmd.maxArgs == common::cli::ParsedCommand::maxArgumentCount;
      if (not forwardsArguments and command.argumentCount == 1 and strcmp(command.argument(0), "h") == 0)
      {
        print_command_description(cmd, calledCommand);
        return;
      }

      // check arguments
      if (command.argumentCount < cmd.minArgs or command.argumentCount > cmd.maxArgs)
      {
        bsp::lampda_print("Invalid call for the command. Check use below:");
        print_command_description(cmd, calledCommand);
        return;
      }

      cmd.handler(command);
      return;
    }
  }

  if (calledCommand)
  {
    bsp::lampda_print("unknown command: \'%s %s\'", calledCommand, command.name());
    bsp::lampda_print("type \'%s h\' for available commands", calledCommand);
  }
  else
  {
    bsp::lampda_print("unknown command: \'%s\'", command.name());
    bsp::lampda_print("type \'h\' for available commands");
  }
}

namespace handles {

void help_hook();
/// Provide an info/help page
static void cmd_help(const common::cli::ParsedCommand&) { help_hook(); }

/// Print the 64 bits serial number as 16 hex digits.
/// The newlib-nano printf on nRF52 has no 64 bits integer support, so print it as two 32 bits halves.
static void print_serial_number()
{
  const uint64_t serialNumber = hal::registers::get_device_serial_number();
  bsp::lampda_print("SN: %08lX%08lX",
                    static_cast<unsigned long>(serialNumber >> 32u),
                    static_cast<unsigned long>(serialNumber & 0xffffffffu));
}

/// Short system status summary
static void cmd_summary(const common::cli::ParsedCommand&)
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
  bsp::lampda_print("Bluetooth: %s", bsp::ble::is_connected() ? "Connected" : "Disconnected");
  // Temperature
  bsp::lampda_print("Temp: %.1f°C", hal::registers::read_CPU_temperature_degreesC());
  // serial number
  print_serial_number();
  bsp::lampda_print("--------------------");

  if (bsp::filesystem::has_overflowed())
  {
    bsp::lampda_print(
            "\n/!\\ File system has overflowed. Some parameters may be lost. Augment the size of the filesystem "
            "static_map to alleviate this\n");
  }
}

/// Check all system versions
static void cmd_version(const common::cli::ParsedCommand&)
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

/// Display the board type
static void cmd_type(const common::cli::ParsedCommand&)
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

/// Display the board unique serial number
static void cmd_id(const common::cli::ParsedCommand&) { print_serial_number(); }

/// Display system statistics
static void cmd_stats(const common::cli::ParsedCommand&) { logic::statistics::show(); }

/// Display battery infos
static void cmd_bat(const common::cli::ParsedCommand&)
{
  const auto& balancerStatus = ::lampda::bsp::balancer::get_status();
  const bool areBalancerValueValid = balancerStatus.is_valid();

  if (areBalancerValueValid)
  {
    // print individual battery voltages
    for (uint8_t i = 0; i < physical_constants::batterySeriesCount; ++i)
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

  bsp::lampda_print("level blinks enabled: %s", boolToString(logic::battery_blink::is_enabled()));

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

/// Display charger component informations
static void cmd_cinfo(const common::cli::ParsedCommand&)
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

/// Display all ADC values
static void cmd_adc(const common::cli::ParsedCommand&)
{
  const auto& chargerState = ::lampda::component::charger::get_state();
  if (chargerState.areMeasuresOk)
  {
    bsp::lampda_print(
            "Last update %lums\n"
            "PowerRail voltage:%dmV\n"
            "PowerRail current:%dmA\n"
            "VBUS voltage:%dmV\n"
            "Bat voltage:%dmV\n"
            "Bat current:%dmA\n"
            "Temperature:%.2fC",
            static_cast<unsigned long>(chargerState.lastUpdateTime_ms),
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
            "Last update %lums\n"
            "VBUS voltage:%dmV\n"
            "Temperature:%.2fC",
            static_cast<unsigned long>(chargerState.lastUpdateTime_ms),
            ::lampda::bsp::powerDelivery::get_vbus_voltage(),
            hal::registers::read_CPU_temperature_degreesC());
  }
}

/// Display the current power delivery capacities
static void cmd_powerdelivery(const common::cli::ParsedCommand&)
{
  ::lampda::bsp::powerDelivery::show_pd_status();
  const auto& pd = ::lampda::bsp::powerDelivery::get_available_pd();

  bool hasValidPDO = false;
  for (const auto& pdo: pd)
  {
    if (pdo.is_valid())
    {
      // First print
      if (not hasValidPDO)
        bsp::lampda_print("Power delivery profiles :");

      bsp::lampda_print("- %lumV, %lumA",
                        static_cast<unsigned long>(pdo.voltage_mv),
                        static_cast<unsigned long>(pdo.maxCurrent_mA));
      hasValidPDO = true;
    }
    else
      break;
  }

  if (not hasValidPDO)
    bsp::lampda_print("No power delivery capabilities");
}

/// Display the system states machines
static void cmd_states(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("behavior machine state:%s. error msgs: %s",
                    logic::behavior::get_state().c_str(),
                    logic::behavior::get_error_state_message().c_str());
  bsp::lampda_print("power state machine state: %s. error msgs: %s",
                    logic::power::get_state().c_str(),
                    logic::power::get_error_string().c_str());
}

static void cmd_recovery(const common::cli::ParsedCommand&) { logic::power::show_recovery_diagnostics(); }
static void cmd_recovery_trace(const common::cli::ParsedCommand&) { logic::power::show_recovery_trace(); }

/// Display all active alerts
static void cmd_alerts(const common::cli::ParsedCommand&)
{
  logic::alerts::show_all();

  // This raises no alerts but should be indicated
  if (bsp::filesystem::has_overflowed())
  {
    bsp::lampda_print(
            "\n/!\\ File system has overflowed. Some parameters may be lost. Augment the size of the filesystem "
            "static_map to alleviate this\n");
  }
}

/// Read i2c activate address
static void cmd_i2c(const common::cli::ParsedCommand&)
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

static void cmd_get_ble_name(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("ble_name: %s", bsp::ble::get_ble_name().data());
}

static void cmd_clear_user_parameters(const common::cli::ParsedCommand&)
{
  // Delete user parameters
  if (bsp::filesystem::user::format())
    bsp::lampda_print("Cleared all user parameters");
  else
    bsp::lampda_print("Error: failed to clear user parameters");
}

/// Format the file system
static void cmd_format(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("clearing the whole file format");
  bsp::filesystem::clear_internal_fs();
}

/// Enter Device Firmware Update mode
static void cmd_dfu(const common::cli::ParsedCommand&) { hal::registers::enter_serial_dfu(); }

/// Toggle the button gpio for the next boot
static void cmd_buttontoggle(const common::cli::ParsedCommand&)
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

/// Shutdown the system
static void cmd_shutdown(const common::cli::ParsedCommand&) { logic::behavior::internal::handle_shutdown_state(); }

/// Debug task usages and activity
static void cmd_tasks(const common::cli::ParsedCommand&) { bsp::threads::display_thread_debug(); }

/// Bluetooth infos
static void cmd_ble(const common::cli::ParsedCommand&)
{
  bsp::lampda_print(
          "is activated: %d\n"
          "is advertising: %d\n"
          "is open to all: %d\n"
          "is connected: %d\n"
          "is msg received: %d\n"
          "auto activations left: %lu",
          bsp::ble::is_activated(),
          bsp::ble::is_advertising(),
          bsp::ble::is_open_to_all(),
          bsp::ble::is_connected(),
          logic::inputs_bluetooth::is_bluetooth_used(),
          static_cast<unsigned long>(logic::behavior::internal::get_bluetooth_auto_activation_left()));

  std::array<uint8_t, 8> boundedDeviceName;
  const bool isBounded = bsp::ble::is_bounded(boundedDeviceName);
  if (isBounded)
  {
    bsp::lampda_print("BLE bounded to device: type=0x%02X %02X:%02X:%02X:%02X:%02X:%02X",
                      boundedDeviceName[0],
                      boundedDeviceName[1],
                      boundedDeviceName[2],
                      boundedDeviceName[3],
                      boundedDeviceName[4],
                      boundedDeviceName[5],
                      boundedDeviceName[6]);
  }
  else
  {
    bsp::lampda_print("BLE not bounded to a device");
  }
}

/// Change the system brightness
static void cmd_brightness(const common::cli::ParsedCommand& command)
{
  brightness_t brightness = 0;
  if (common::cli::argument::parse_uint16(command, 0, brightness) &&
      brightness >= ::lampda::brightness::absoluteMinimumBrightness &&
      brightness <= ::lampda::brightness::absoluteMaximumBrightness)
  {
    lampda::user::handle_user_command(common::UserCommand::make_brightness_command(brightness));
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be in range <%u-%u>",
                    ::lampda::brightness::absoluteMinimumBrightness,
                    ::lampda::brightness::absoluteMaximumBrightness);
}

/// Enable of update the sunset timer
static void cmd_sunset(const common::cli::ParsedCommand& command)
{
  uint16_t timeMinutes = 0;
  if (common::cli::argument::parse_uint16(command, 0, timeMinutes) && timeMinutes > 0)
  {
    // lamp will turn on if not already turned on
    if (not logic::behavior::is_in_output_state())
      logic::behavior::set_power_on();

    logic::sunset_timer.add_time_minutes(timeMinutes);
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be greater than zero");
}

static void cmd_set_sunset_time(const common::cli::ParsedCommand& command)
{
  uint8_t hour;
  uint8_t minute;
  uint8_t seconds;
  uint8_t dayofWeek;
  if (common::cli::argument::parse_uint8(command, 0, hour) && common::cli::argument::parse_uint8(command, 1, minute) &&
      common::cli::argument::parse_uint8(command, 2, seconds) &&
      common::cli::argument::parse_uint8(command, 3, dayofWeek))
  {
    component::time::RealTime time;
    time.hour = hour;
    time.minutes = minute;
    time.seconds = seconds;
    time.dayOfTheWeek = dayofWeek;
    if (time.is_valid())
    {
      lampda::user::handle_user_command(common::UserCommand::make_set_sunset_to_time_command(time));
      return;
    }
  }
  bsp::lampda_print("Invalid call: parameters should be range <0-23> <0-59> <0-59> <0-6>");
}

/// Enable of update the sunrise timer
static void cmd_sunrise(const common::cli::ParsedCommand& command)
{
  uint16_t timeMinutes = 0;
  if (common::cli::argument::parse_uint16(command, 0, timeMinutes) && timeMinutes > 0)
  {
    logic::sunrise_timer.add_time_minutes(timeMinutes);
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be greater than zero");
}

static void cmd_set_sunrise_time(const common::cli::ParsedCommand& command)
{
  uint8_t hour;
  uint8_t minute;
  uint8_t seconds;
  uint8_t dayofWeek;
  if (common::cli::argument::parse_uint8(command, 0, hour) && common::cli::argument::parse_uint8(command, 1, minute) &&
      common::cli::argument::parse_uint8(command, 2, seconds) &&
      common::cli::argument::parse_uint8(command, 3, dayofWeek))
  {
    component::time::RealTime time;
    time.hour = hour;
    time.minutes = minute;
    time.seconds = seconds;
    time.dayOfTheWeek = dayofWeek;
    if (time.is_valid())
    {
      lampda::user::handle_user_command(common::UserCommand::make_set_sunrise_to_time_command(time));
      return;
    }
  }
  bsp::lampda_print("Invalid call: parameters should be range <0-23> <0-59> <0-59> <0-6>");
}

/// Set the user ramp to a target value
static void cmd_ramp(const common::cli::ParsedCommand& command)
{
  uint8_t ramp = 0;
  if (common::cli::argument::parse_uint8(command, 0, ramp))
  {
    lampda::user::handle_user_command(common::UserCommand::make_set_ramp_command(ramp));
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be range <0-255>");
}

static void cmd_set_mode(const common::cli::ParsedCommand& command)
{
  uint8_t pageIndex = 0;
  uint8_t modeIndex = 0;
  if (common::cli::argument::parse_uint8(command, 0, pageIndex) &&
      common::cli::argument::parse_uint8(command, 1, modeIndex))
  {
    lampda::user::handle_user_command(common::UserCommand::make_set_mode_command(pageIndex, modeIndex));
    return;
  }
  else
  {
    bsp::lampda_print("Invalid call: parameters should be range <0-255> <0-255>");
  }
}

static void cmd_onoff(const common::cli::ParsedCommand& command)
{
  uint8_t onOff;
  if (common::cli::argument::parse_uint8(command, 0, onOff) && onOff <= 1)
  {
    lampda::user::handle_user_command(common::UserCommand::make_turn_onoff_command(onOff != 0));
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be range <0-1>");
}

static void cmd_set_battery_blink(const common::cli::ParsedCommand& command)
{
  uint8_t isEnabled;
  if (common::cli::argument::parse_uint8(command, 0, isEnabled) && isEnabled <= 1)
  {
    logic::battery_blink::set_enabled(isEnabled != 0);
    bsp::lampda_print("Battery level blinks: %s", boolToString(isEnabled != 0));
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be range <0-1>");
}

static void cmd_set_real_time(const common::cli::ParsedCommand& command)
{
  uint8_t hour;
  uint8_t minute;
  uint8_t seconds;
  uint8_t dayofWeek;
  if (common::cli::argument::parse_uint8(command, 0, hour) && common::cli::argument::parse_uint8(command, 1, minute) &&
      common::cli::argument::parse_uint8(command, 2, seconds) &&
      common::cli::argument::parse_uint8(command, 3, dayofWeek))
  {
    component::time::RealTime time;
    time.hour = hour;
    time.minutes = minute;
    time.seconds = seconds;
    time.dayOfTheWeek = dayofWeek;
    if (time.is_valid())
    {
      lampda::user::handle_user_command(common::UserCommand::make_set_real_time_command(time));
      return;
    }
  }
  bsp::lampda_print("Invalid call: parameters should be range <0-23> <0-59> <0-59> <0-6>");
}

/// Display real time
static void cmd_time(const common::cli::ParsedCommand&)
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

/// Display serial port infos
static void cmd_serial(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("Local serial port: active %d", hal::serial::is_activated());
  bsp::lampda_print("BLE serial port: active %d", bsp::ble::serial::is_activated());
}

static void cmd_set_ble_name(const common::cli::ParsedCommand& command)
{
  auto charValidator = [](const char c) {
    // Allow numbers
    if (c >= '0' && c <= '9')
      return true;
    // Allow lower case letters
    if (c >= 'a' && c <= 'z')
      return true;
    // Allow upper case letters
    if (c >= 'A' && c <= 'Z')
      return true;
    // special alowed chars
    if (c == '-')
      return true;

    // invalid char
    return false;
  };

  std::array<char, common::cli::MaxStringArgumentLen> name;
  if (common::cli::argument::parse_text(command, 0, name))
  {
    uint8_t realNameSize = 0;
    for (uint8_t i = 0; i < common::cli::MaxStringArgumentLen; i++)
    {
      if (name[i] == '\0')
        break;
      if (not charValidator(name[i]))
      {
        bsp::lampda_print("BLE name contains invalid characters. Allowed chars are: letters, numbers, and -");
        return;
      }
      realNameSize++;
    }
    if (realNameSize >= common::cli::MaxStringArgumentLen - 1)
    {
      bsp::lampda_print("New ble name is too long, max is %d characters",
                        static_cast<int>(common::cli::MaxStringArgumentLen - 2));
      return;
    }

    bsp::ble::set_bluetooth_name(name);
    bsp::lampda_print("New ble name set to: %s, will be set after a reboot", name.data());
  }
  else
  {
    bsp::lampda_print("Could not parse the given name");
  }
}

static void cmd_set_go_to_favorite(const common::cli::ParsedCommand& command)
{
  uint8_t index;
  if (common::cli::argument::parse_uint8(command, 0, index))
  {
    lampda::user::handle_user_command(common::UserCommand::make_go_to_favorite_command(index));
  }
  else
  {
    bsp::lampda_print("Invalid call: parameters should be range <0-255>");
  }
}

static void cmd_set_current_as_favorite(const common::cli::ParsedCommand& command)
{
  uint8_t index;
  if (common::cli::argument::parse_uint8(command, 0, index))
  {
    lampda::user::handle_user_command(common::UserCommand::make_set_favorite_command(index));
  }
  else
  {
    bsp::lampda_print("Invalid call: parameters should be range <0-255>");
  }
}

static void cmd_set_custom_color(const common::cli::ParsedCommand& command)
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  if (common::cli::argument::parse_uint8(command, 0, red) && common::cli::argument::parse_uint8(command, 1, green) &&
      common::cli::argument::parse_uint8(command, 2, blue))
  {
    lampda::user::handle_user_command(common::UserCommand::make_set_ble_custom_color_mode_command(red, green, blue));
  }
  else
  {
    bsp::lampda_print("Invalid call: parameters should be range <0-255> <0-255> <0-255>");
  }
}

/// Set the indicator brightness level (saved)
static void cmd_set_indicator_level(const common::cli::ParsedCommand& command)
{
  uint8_t level;
  if (common::cli::argument::parse_uint8(command, 0, level) && level <= 2)
  {
    logic::indicator::set_brightness_level(level);
    bsp::lampda_print("Indicator level set to %d", level);
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be range <0-2>");
}

/// Set the indicator color, or restore the battery level color without parameters
static void cmd_set_indicator_color(const common::cli::ParsedCommand& command)
{
  if (command.argumentCount == 0)
  {
    logic::indicator::clear_color_override();
    bsp::lampda_print("Indicator color restored to battery level");
    return;
  }

  uint8_t red;
  uint8_t green;
  uint8_t blue;
  if (command.argumentCount == 3 && common::cli::argument::parse_uint8(command, 0, red) &&
      common::cli::argument::parse_uint8(command, 1, green) && common::cli::argument::parse_uint8(command, 2, blue))
  {
    logic::indicator::set_color_override(red, green, blue);
    bsp::lampda_print("Indicator color set to %d %d %d (alerts still take precedence)", red, green, blue);
    return;
  }
  bsp::lampda_print("Invalid call: parameters should be none, or range <0-255> <0-255> <0-255>");
}

/// Display the indicator state
static void cmd_indicator(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("indicator level: %d", logic::indicator::get_brightness_level());

  uint32_t color = 0;
  if (logic::indicator::get_color_override(color))
    bsp::lampda_print("indicator color: %d %d %d",
                      static_cast<int>((color >> 16) & 0xFFu),
                      static_cast<int>((color >> 8) & 0xFFu),
                      static_cast<int>(color & 0xFFu));
  else
    bsp::lampda_print("indicator color: battery level");
}

/// Set the microphone sensitivity (not saved)
static void cmd_set_mic_sensitivity(const common::cli::ParsedCommand& command)
{
  uint8_t sensitivity;
  if (common::cli::argument::parse_uint8(command, 0, sensitivity) &&
      sensitivity <= component::microphone::maxSensitivity)
  {
    component::microphone::set_sensitivity(sensitivity);
    bsp::lampda_print("Microphone sensitivity set to %d (gain x%.2f)",
                      sensitivity,
                      component::microphone::sensitivity_to_gain(sensitivity));
    return;
  }
  bsp::lampda_print("Invalid call: parameter should be range <0-%d>", component::microphone::maxSensitivity);
}

/// Display the microphone state
static void cmd_mic(const common::cli::ParsedCommand&)
{
  const uint8_t sensitivity = component::microphone::get_sensitivity();
  bsp::lampda_print(
          "mic sensitivity: %d (gain x%.2f)", sensitivity, component::microphone::sensitivity_to_gain(sensitivity));
}

/// Display the links between click counts and behaviors
static void cmd_bindings(const common::cli::ParsedCommand&)
{
  namespace bindings = logic::button_bindings;

  for (size_t i = 0; i < bindings::behaviorCount; ++i)
  {
    const auto behavior = static_cast<bindings::Behavior>(i);
    if (not bindings::is_supported(behavior))
      continue;

    // display the actions really linked: <n>C for the click action, <n>H for the hold action
    const uint8_t clicks = bindings::get_clicks(behavior);
    const bool isOnClick = bindings::get_click_behavior(clicks) == behavior;
    const bool isOnHold = bindings::get_hold_behavior(clicks) == behavior;

    char actions[12] = "";
    if (isOnClick and isOnHold)
      snprintf(actions, sizeof(actions), "%dC, %dH", clicks, clicks);
    else if (isOnClick)
      snprintf(actions, sizeof(actions), "%dC", clicks);
    else if (isOnHold)
      snprintf(actions, sizeof(actions), "%dH", clicks);
    else
      snprintf(actions, sizeof(actions), "none");

    bsp::lampda_print("%s: %s (can use %d to %d)",
                      bindings::get_name(behavior),
                      actions,
                      bindings::get_min_clicks(behavior),
                      bindings::maxClicks);
  }
}

/// Link a behavior to a click count
static void cmd_set_binding(const common::cli::ParsedCommand& command)
{
  namespace bindings = logic::button_bindings;

  std::array<char, common::cli::MaxStringArgumentLen> name;
  uint8_t clicks = 0;
  if (not common::cli::argument::parse_text(command, 0, name) or
      not common::cli::argument::parse_uint8(command, 1, clicks))
  {
    bsp::lampda_print("Invalid call: parameters should be <behavior> <clicks>");
    return;
  }

  const auto behavior = bindings::get_behavior_from_name(name.data());
  if (behavior == bindings::Behavior::None)
  {
    bsp::lampda_print(R"(Unknown behavior "%s": type 'sys bind' for the behaviors)", name.data());
    return;
  }

  std::array<uint8_t, bindings::behaviorCount> previousClicks;
  for (size_t i = 0; i < bindings::behaviorCount; ++i)
    previousClicks[i] = bindings::get_clicks(static_cast<bindings::Behavior>(i));

  switch (bindings::set_binding(behavior, clicks))
  {
    case bindings::SetResult::Success:
      bsp::lampda_print("%s is now on %d clicks", name.data(), clicks);
      // report the behaviors that swapped
      for (size_t i = 0; i < bindings::behaviorCount; ++i)
      {
        const auto other = static_cast<bindings::Behavior>(i);
        if (other != behavior and bindings::get_clicks(other) != previousClicks[i])
          bsp::lampda_print("%s moved to %d clicks", bindings::get_name(other), bindings::get_clicks(other));
      }
      break;
    case bindings::SetResult::UnsupportedBehavior:
      bsp::lampda_print("This lamp type does not have the %s behavior", name.data());
      break;
    case bindings::SetResult::ReservedClicks:
      bsp::lampda_print("%s cannot use %d clicks: should be %d to %d",
                        name.data(),
                        clicks,
                        bindings::get_min_clicks(behavior),
                        bindings::maxClicks);
      break;
    case bindings::SetResult::SwapNotPossible:
      bsp::lampda_print("Cannot move %s to %d clicks: the behaviors there cannot swap with it. Move them first",
                        name.data(),
                        clicks);
      break;
  }
}

/// Restore the default links between click counts and behaviors
static void cmd_reset_bindings(const common::cli::ParsedCommand&)
{
  logic::button_bindings::reset_to_defaults();
  bsp::lampda_print("Restored the default button bindings");
}

/// Display the state of a sun timer
static void print_sun_timer_state(const char* name, const bool isEnabled, const uint32_t remainingTime_s)
{
  if (isEnabled)
    bsp::lampda_print("%s: enabled, ends in %lum %lus",
                      name,
                      static_cast<unsigned long>(remainingTime_s / 60),
                      static_cast<unsigned long>(remainingTime_s % 60));
  else
    bsp::lampda_print("%s: disabled", name);
}

/// Display the sunset and sunrise timers states
static void cmd_sun_timers(const common::cli::ParsedCommand&)
{
  print_sun_timer_state("sunset", logic::sunset_timer.is_enabled(), logic::sunset_timer.get_remaining_time_s());
  print_sun_timer_state("sunrise", logic::sunrise_timer.is_enabled(), logic::sunrise_timer.get_remaining_time_s());
}

/// Cancel the active sun timers
static void cmd_cancel_sun_timers(const common::cli::ParsedCommand&)
{
  // the timers are exclusive: cancel only the running one, the timer task sees the cancel and skips its end behavior
  if (logic::sunset_timer.is_enabled())
  {
    logic::sunset_timer.cancel();
    bsp::lampda_print("Sunset timer canceled");
    return;
  }
  if (logic::sunrise_timer.is_enabled())
  {
    // keep the brightness reached by the sunrise, as for a user brightness change
    logic::sunrise_timer.cancel_at_current_brightness();
    bsp::lampda_print("Sunrise timer canceled");
    return;
  }
  bsp::lampda_print("No sun timer is running");
}

/// Display if the battery level blinks are enabled
static void cmd_battery_blink(const common::cli::ParsedCommand&)
{
  bsp::lampda_print("level blinks enabled: %s", boolToString(logic::battery_blink::is_enabled()));
}

/// Display the favorite modes infos
static void cmd_favorites(const common::cli::ParsedCommand&)
{
  lampda::user::handle_user_command(common::UserCommand::make_print_favorites_command());
}

/// Display a single line status, for external apps
static void cmd_status(const common::cli::ParsedCommand&)
{
  lampda::user::handle_user_command(common::UserCommand::make_print_status_command());
}

void cmd_set_hook(const common::cli::ParsedCommand& command, const char* name);
/// Handle the Set command, that takes another command as parameters
static void cmd_set(const common::cli::ParsedCommand& command) { cmd_set_hook(command, command.name()); }

void cmd_sys_hook(const common::cli::ParsedCommand& command, const char* name);
/// Handle the Sys command, that takes another command as parameters
static void cmd_sys(const common::cli::ParsedCommand& command) { cmd_sys_hook(command, command.name()); }

void cmd_act_hook(const common::cli::ParsedCommand& command, const char* name);
/// Handle the Act command, that takes another command as parameters
static void cmd_act(const common::cli::ParsedCommand& command) { cmd_act_hook(command, command.name()); }

void cmd_debug_hook(const common::cli::ParsedCommand& command, const char* name);
/// Handle the Debug command, that takes another command as parameters
static void cmd_debug(const common::cli::ParsedCommand& command) { cmd_debug_hook(command, command.name()); }

void set_help_hook();
/// Handle the Set help command
static void cmd_set_help(const common::cli::ParsedCommand&) { set_help_hook(); }

void sys_help_hook();
/// Handle the Sys help command
static void cmd_sys_help(const common::cli::ParsedCommand&) { sys_help_hook(); }

void act_help_hook();
/// Handle the act help command
static void cmd_act_help(const common::cli::ParsedCommand&) { act_help_hook(); }

void debug_help_hook();
/// Handle the debug help command
static void cmd_debug_help(const common::cli::ParsedCommand&) { debug_help_hook(); }

} // namespace handles

/// Define the CLI commands here
constexpr Command _main_commands_s[] = {
        make_command("h", "this page", handles::cmd_help),
        make_command("sum", "system summary", handles::cmd_summary),
        make_command("v", "hardware & software version", handles::cmd_version),
        make_command("t", "return the lamp type", handles::cmd_type),
        make_command("id", "return the board serial number", handles::cmd_id),
        make_command_args("set",
                          "<subcommand> [<arg>...]",
                          0,
                          common::cli::ParsedCommand::maxArgumentCount,
                          "Set system characteristics",
                          handles::cmd_set),
        make_command_args("sys",
                          "<subcommand> [<arg>...]",
                          0,
                          common::cli::ParsedCommand::maxArgumentCount,
                          "Read system informations",
                          handles::cmd_sys),
        make_command_args("debug",
                          "<subcommand> [<arg>...]",
                          0,
                          common::cli::ParsedCommand::maxArgumentCount,
                          "Low level debug informations",
                          handles::cmd_debug),
        make_command_args("act",
                          "<subcommand> [<arg>...]",
                          0,
                          common::cli::ParsedCommand::maxArgumentCount,
                          "High level immediate actions (dangerous)",
                          handles::cmd_act),
};

/// special commands to interact with the low level system
constexpr Command _sys_commands_s[] = {
        make_command_args("h", "", 0, common::cli::ParsedCommand::maxArgumentCount, "This page", handles::cmd_sys_help),
        make_command("alerts", "show all raised alerts", handles::cmd_alerts),
        make_command("time", "show current time", handles::cmd_time),
        make_command("bat", " battery info/levels", handles::cmd_bat),
        make_command("ble_name", "return the current BLE device name", handles::cmd_get_ble_name),
        make_command("bindings", "show which clicks trigger each configurable behavior", handles::cmd_bindings),
        make_command("sun", "show the sunset and sunrise timers states", handles::cmd_sun_timers),
        make_command("fav", "show the favorite mode count", handles::cmd_favorites),
        make_command("status",
                     "one line status: $stat:g<group>:m<mode>:r<ramp>:<F if in favorites, else X> brgt:<brightness>",
                     handles::cmd_status),
        make_command("bat_blink", "show if the battery level blinks are enabled", handles::cmd_battery_blink),
        make_command("mic", "show the microphone sensitivity", handles::cmd_mic),
};

/// commands to debug the low level system: tasks, power, recovery, electrical buses
constexpr Command _debug_commands_s[] = {
        make_command_args(
                "h", "", 0, common::cli::ParsedCommand::maxArgumentCount, "This page", handles::cmd_debug_help),
        make_command("tasks", "display a debug of task usages", handles::cmd_tasks),
        make_command("ble", "debug bluetooth informations", handles::cmd_ble),
        make_command("pd", "display the connected PD capabilities", handles::cmd_powerdelivery),
        make_command("states", "state machine states", handles::cmd_states),
        make_command("recovery", "read-only startup and battery recovery diagnostics", handles::cmd_recovery),
        make_command("rtrace", "read-only recovery trace only", handles::cmd_recovery_trace),
        make_command("serial", "show serial infos", handles::cmd_serial),
        make_command("cinfo", "charger infos", handles::cmd_cinfo),
        make_command("adc", "values from the charger ADC", handles::cmd_adc),
        make_command("i2c", "start an i2c presence check", handles::cmd_i2c),
        make_command("stats", "display the system use statistics", handles::cmd_stats),
        make_command("bind", "show which clicks trigger each configurable behavior", handles::cmd_bindings),
        make_command("ind", "show the indicator level and color", handles::cmd_indicator),
};

/// special commands to interact with the low level system
constexpr Command _act_commands_s[] = {
        make_command_args("h",
                          "",
                          0,
                          common::cli::ParsedCommand::maxArgumentCount,
                          "This page. Dangerous commands /!\\",
                          handles::cmd_act_help),
        make_command("shutdown", "force shutdown the system", handles::cmd_shutdown),
        make_command("buttonTogg", "change the button gpio", handles::cmd_buttontoggle),
        make_command("clear_user", "Clear all of the user parameters", handles::cmd_clear_user_parameters),
        make_command("format", "format the whole file system (not recommanded !!)", handles::cmd_format),
        make_command("dfu", "clear this program from memory, enter update mode", handles::cmd_dfu),
};

/// special commands to set system values
constexpr Command _set_commands_s[] = {
        make_command_args("h", "", 0, common::cli::ParsedCommand::maxArgumentCount, "This page", handles::cmd_set_help),
        make_command_args("brgt",
                          "[0-1024]", // should be [::lampda::brightness::absoluteMinimumBrightness;
                                      // ::lampda::brightness::absoluteMaximumBrightness]
                          1,
                          1,
                          "Set the output brightness",
                          handles::cmd_brightness),
        make_command_args("ramp", "[0-255]", 1, 1, "Set the user ramp value", handles::cmd_ramp),
        make_command_args("mode", "[group id] [mode id]", 2, 2, "Set current active mode", handles::cmd_set_mode),
        make_command_args("on", "[0-1]", 1, 1, "Turn system on or off", handles::cmd_onoff),
        make_command_args("bat_blink",
                          "[0-1]",
                          1,
                          1,
                          "Enable or disable the output blinks at 75%, 50% and 25% of battery",
                          handles::cmd_set_battery_blink),
        make_command_args("time",
                          "[0-23](hour) [0-59](minute) [0-59](second) [0-6](day)",
                          4,
                          4,
                          "Set system real time",
                          handles::cmd_set_real_time),
        make_command_args("sunset_min",
                          "[0-10](minutes)",
                          1,
                          1,
                          "Set the sunset timer, or add time if already started.",
                          handles::cmd_sunset),
        make_command_args("sunset",
                          "[0-23](hour) [0-59](minute) [0-59](second) [0-6](day)",
                          4,
                          4,
                          "Set the sunset timer to a time, if time is synchronised",
                          handles::cmd_set_sunset_time),
        make_command_args("sunrise_min",
                          "[0-10](minutes)",
                          1,
                          1,
                          "Set the sunrise timer, or add time if already started.",
                          handles::cmd_sunrise),
        make_command_args("sunrise",
                          "[0-23](hour) [0-59](minute) [0-59](second) [0-6](day)",
                          4,
                          4,
                          "Set the sunrise timer to a time, if time is synchronised",
                          handles::cmd_set_sunrise_time),
        make_command("sun_cancel", "Cancel the running sunset or sunrise timer", handles::cmd_cancel_sun_timers),
        make_command_args("ble_name",
                          "[New name]",
                          1,
                          1,
                          "Set the new bluetooth advertised name. It should start with \'ELK-BLE-\" if you want to use "
                          "a generic ELK compatible app",
                          handles::cmd_set_ble_name),
        make_command_args("favorite",
                          "[0-15](index)",
                          1,
                          1,
                          "Go to the given favorite index, if possible",
                          handles::cmd_set_go_to_favorite),
        make_command_args("new_favorite",
                          "[0-15](index)",
                          1,
                          1,
                          "Set the current mode to the given favorite index, if possible",
                          handles::cmd_set_current_as_favorite),
        make_command_args("color",
                          "[0-255](red) [0-255](green) [0-255](blue)",
                          3,
                          3,
                          "Display a given color",
                          handles::cmd_set_custom_color),
        make_command_args("bind",
                          "[favorite|nav|scroll|sunset|ramp|indicator] [clicks]",
                          2,
                          2,
                          "Move a behavior, with its click and hold actions. Swaps with the behaviors already there",
                          handles::cmd_set_binding),
        make_command("bind_reset", "Restore the default button bindings", handles::cmd_reset_bindings),
        make_command_args("ind_lvl",
                          "[0-2]",
                          1,
                          1,
                          "Set the indicator brightness (saved). 0: full, 1: low, 2: low and off when charging",
                          handles::cmd_set_indicator_level),
        make_command_args("ind_color",
                          "[0-255](red) [0-255](green) [0-255](blue)",
                          0,
                          3,
                          "Set the indicator color, alerts still take precedence. No parameters restores the battery "
                          "level color",
                          handles::cmd_set_indicator_color),
        make_command_args("mic_sens",
                          "[0-100]",
                          1,
                          1,
                          "Set the microphone sensitivity (not saved). 50: no gain, 0: gain x0.1, 100: gain x10",
                          handles::cmd_set_mic_sensitivity),

};

/// Check for command duplication
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
static_assert(!has_duplicate_hash(_main_commands_s, sizeof(_main_commands_s) / sizeof(_main_commands_s[0])),
              "Duplicate command hash detected! Ensure all command names are unique.");
/// Check for command uniqueness
static_assert(!has_duplicate_hash(_set_commands_s, sizeof(_set_commands_s) / sizeof(_set_commands_s[0])),
              "Duplicate set command hash detected! Ensure all command names are unique.");
/// Check for command uniqueness
static_assert(!has_duplicate_hash(_sys_commands_s, sizeof(_sys_commands_s) / sizeof(_sys_commands_s[0])),
              "Duplicate sys command hash detected! Ensure all command names are unique.");
/// Check for command uniqueness
static_assert(!has_duplicate_hash(_act_commands_s, sizeof(_act_commands_s) / sizeof(_act_commands_s[0])),
              "Duplicate act command hash detected! Ensure all command names are unique.");
/// Check for command uniqueness
static_assert(!has_duplicate_hash(_debug_commands_s, sizeof(_debug_commands_s) / sizeof(_debug_commands_s[0])),
              "Duplicate debug command hash detected! Ensure all command names are unique.");

/// hook that can read the other commands and print them out
template<size_t N> void help_hook_base(const Command (&commandsToParse)[N], const char* command = nullptr)
{
  if (command)
    bsp::lampda_print("---Lamp-da CLI: %s---", command);
  else
    bsp::lampda_print("---Lamp-da CLI---");

  for (const auto& cmd: commandsToParse)
  {
    if (command)
    {
      if (cmd.usage)
        bsp::lampda_print("%s %s %s", command, cmd.name, cmd.usage);
      else
        bsp::lampda_print("%s %s", command, cmd.name);
    }
    else
    {
      if (cmd.usage)
        bsp::lampda_print("%s %s", cmd.name, cmd.usage);
      else
        bsp::lampda_print("%s", cmd.name);
    }
    // let the thread rest, this command list can be big
    hal::delay_ms(20);
  }
  bsp::lampda_print("-----------------");
}

namespace handles {
void help_hook() { help_hook_base(_main_commands_s); }

void set_help_hook() { help_hook_base(_set_commands_s, "set"); }

void sys_help_hook() { help_hook_base(_sys_commands_s, "sys"); }

void act_help_hook() { help_hook_base(_act_commands_s, "act"); }

void debug_help_hook() { help_hook_base(_debug_commands_s, "debug"); }

void cmd_set_hook(const common::cli::ParsedCommand& command, const char* name)
{
  if (command.argumentCount < 1)
  {
    set_help_hook();
    return;
  }

  handleCommand(command.shift_to_first_parameter(), _set_commands_s, name);
}

void cmd_sys_hook(const common::cli::ParsedCommand& command, const char* name)
{
  if (command.argumentCount < 1)
  {
    sys_help_hook();
    return;
  }

  handleCommand(command.shift_to_first_parameter(), _sys_commands_s, name);
}

void cmd_act_hook(const common::cli::ParsedCommand& command, const char* name)
{
  if (command.argumentCount < 1)
  {
    act_help_hook();
    return;
  }

  handleCommand(command.shift_to_first_parameter(), _act_commands_s, name);
}

void cmd_debug_hook(const common::cli::ParsedCommand& command, const char* name)
{
  if (command.argumentCount < 1)
  {
    debug_help_hook();
    return;
  }

  handleCommand(command.shift_to_first_parameter(), _debug_commands_s, name);
}

} // namespace handles

void setup() { bsp::lampda_print_init(); }

void handleSerialEvents()
{
  const auto& inputs = bsp::text_in::read_inputs();
  for (size_t i = 0; i < min<uint8_t>(bsp::text_in::Inputs::maxCommands, inputs.commandCount); i++)
  {
    const bsp::text_in::Inputs::Command& input = inputs.commandList[i];

    const auto& command = common::cli::parseCommand(input);
    if (command.name()[0] == '\0')
      continue;
    handleCommand(command, _main_commands_s);
  }
}

} // namespace cli
} // namespace logic
} // namespace lampda
