#include "serial.h"

#include "src/system/charger/charger.h"
#include "src/system/physical/battery.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/print.h"

namespace serial {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

inline const char* const boolToString(bool b) { return b ? "true" : "false"; }

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
                "bl: battery levels\n"
                "cinfo: charger infos\n"
                "ADC: values from the charger ADC\n"
                "cen: enable charger. Debug only\n"
                "format-fs: format the whole file system (dangerous)\n"
                "-----------------");
        break;
      }

    case utils::hash("v"):
      {
        lampda_print(
                "hardware:%s\n"
                "base software:%s\n"
                "user software:%s",
                HARDWARE_VERSION,
                BASE_SOFTWARE_VERSION,
                SOFTWARE_VERSION);
        break;
      }

    case utils::hash("bl"):
      {
        lampda_print(
                "raw battery level:%f%%\n"
                "battery level:%f%%",
                battery::get_raw_battery_level() / 100.0,
                battery::get_battery_level() / 100.0);
        break;
      }

    case utils::hash("cinfo"):
      {
        const auto& chargerState = charger::get_state();

        lampda_print(
                "voltage on vbus:%dmV\n"
                "input current:%dmA\n"
                "battery voltage:%dmV\n"
                "charge current:%dmA\n"
                "is usb powered:%s\n"
                "is charging:%s\n"
                "battery level:%f%%\n"
                "-> %s",
                chargerState.vbus_mV,
                chargerState.inputCurrent_mA,
                chargerState.batteryVoltage_mV,
                chargerState.chargeCurrent_mA,
                boolToString(charger::is_vbus_signal_detected()),
                boolToString(chargerState.is_charging()),
                battery::get_battery_level() / 100.0,
                chargerState.get_status_str().c_str());
        break;
      }

    case utils::hash("cen"):
      lampda_print("Enabling the charging process");
      charger::set_enable_charge(true);
      break;

    case utils::hash("cdis"):
      lampda_print("Disabling the charging process");
      charger::set_enable_charge(false);
      break;

    case utils::hash("ADC"):
      {
        const auto& chargerState = charger::get_state();
        lampda_print(
                "VBUS voltage:%dmV\n"
                "VBUS current:%dmA\n"
                "Bat voltage:%dmV\n",
                "Bat current:%dmA",
                chargerState.vbus_mV,
                chargerState.inputCurrent_mA,
                chargerState.batteryVoltage_mV,
                chargerState.batteryCurrent_mA);
        break;
      }

    case utils::hash("format-fs"):
      lampda_print("clearing the whole file format");
      fileSystem::clear_internal_fs();
      break;

    default:
      lampda_print("unknown command: %s", command);
      lampda_print("type h for available commands");
      break;
  }
}

String inputString = "";

void setup()
{
  init_prints();

  inputString.reserve(255);
}

void handleSerialEvents()
{
  const auto& inputs = read_inputs();
  for (const std::string& input: inputs)
  {
    handleCommand(input);
  }
}

} // namespace serial
