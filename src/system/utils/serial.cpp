#include "serial.h"

#include "constants.h"
#include "src/system/power/power_handler.h"
#include "src/system/power/charger.h"
#include "src/system/power/balancer.h"
#include "src/system/behavior.h"

#include "src/system/physical/battery.h"
#include "src/system/physical/fileSystem.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/print.h"

#include "src/system/alerts.h"

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
                "bat: battery info/levels\n"
                "cinfo: charger infos\n"
                "ADC: values from the charger ADC\n"
                "power: power state machine states\n"
                "alerts: show all raised alerts\n"
                "format-fs: format the whole file system (dangerous)\n"
                "-----------------");
        break;
      }

    case utils::hash("v"):
      {
        lampda_print(
                "hardware:%d.%d\n"
                "firmware:%d.%d\n"
                "user software:%s",
                HARDWARE_VERSION_MAJOR,
                HARDWARE_VERSION_MINOR,
                EXPECTED_FIRMWARE_VERSION_MAJOR,
                EXPECTED_FIRMWARE_VERSION_MINOR,
                SOFTWARE_VERSION);
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
            lampda_print("batt %d: %d mV", i, balancerStatus.batteryVoltages_mV[i]);
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

        if (areChargerValueValid and areBalancerValueValid)
        {
          // print individual battery voltages
          lampda_print(
                  "raw battery level:%f%%\n"
                  "battery level:%f%%",
                  battery::get_level_percent(battery::get_raw_battery_voltage_mv()) / 100.0,
                  battery::get_battery_level() / 100.0);
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
                "battery level:%f%%\n"
                "-> %s",
                boolToString(chargerState.isChargeOkSignalHigh),
                chargerState.vbus_mV,
                chargerState.inputCurrent_mA,
                chargerState.batteryVoltage_mV,
                chargerState.chargeCurrent_mA,
                boolToString(charger::is_vbus_signal_detected()),
                boolToString(chargerState.is_charging()),
                boolToString(chargerState.is_effectivly_charging()),
                battery::get_battery_level() / 100.0,
                chargerState.get_status_str().c_str());
        break;
      }

    case utils::hash("alerts"):
      alerts::show_all();
      break;

    case utils::hash("ADC"):
      {
        const auto& chargerState = charger::get_state();
        lampda_print(
                "VBUS voltage:%dmV\n"
                "VBUS current:%dmA\n"
                "Bat voltage:%dmV\n"
                "Bat current:%dmA",
                chargerState.vbus_mV,
                chargerState.inputCurrent_mA,
                chargerState.batteryVoltage_mV,
                chargerState.batteryCurrent_mA);
        break;
      }

    case utils::hash("power"):
      {
        lampda_print(
                "state machine state:%s\n"
                "behavior machine state:%s",
                power::get_state().c_str(),
                behavior::get_state().c_str());
        break;
      }

    case utils::hash("format-fs"):
      lampda_print("clearing the whole file format");
      fileSystem::clear_internal_fs();
      break;

    default:
      lampda_print("unknown command: %s", command.c_str());
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
