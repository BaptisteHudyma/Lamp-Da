#include "serial.h"

#include <Arduino.h>

#include "src/system/charger/charger.h"
#include "src/system/physical/battery.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace serial {

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

inline const char* const boolToString(bool b) { return b ? "true" : "false"; }

void handleCommand(const String& command) {
  Serial.println("");

  switch (utils::hash(command.c_str())) {
    case utils::hash("h"):
    case utils::hash("help"):
      Serial.println("");
      Serial.println("---Lamp-da CLI---");
      Serial.println("h: this page");
      Serial.println("v: hardware & software version");
      Serial.println("bl: battery level");
      Serial.println("vbus: USB voltage bus infos");
      Serial.println("format-fs: format the whole file system (dangerous)");
      Serial.println("-----------------");
      break;

    case utils::hash("v"):
    case utils::hash("V"):
    case utils::hash("version"):
      Serial.print("hardware:");
      Serial.println(HARDWARE_VERSION);
      Serial.print("base software:");
      Serial.println(BASE_SOFTWARE_VERSION);
      Serial.print("user software:");
      Serial.println(SOFTWARE_VERSION);
      break;

    case utils::hash("bl"):
    case utils::hash("battery"):
      Serial.print("raw battery level:");
      Serial.print(battery::get_raw_battery_level());
      Serial.println("%");
      Serial.print("battery level:");
      Serial.print(battery::get_battery_level());
      Serial.println("%");
      break;

    case utils::hash("vbus"): {
      const auto& chargerState = charger::get_state();
      Serial.print("voltage on vbus:");
      Serial.print(chargerState.vbus_mV);
      Serial.println("mV");
      Serial.print("charge current:");
      Serial.print(chargerState.chargeCurrent_mA);
      Serial.println("mA");
      Serial.print("is usb powered:");
      Serial.println(boolToString(utils::is_powered_with_vbus()));
      Serial.print("is charging:");
      Serial.println(boolToString(chargerState.is_charging()));
      Serial.print("battery level:");
      Serial.print(battery::get_battery_level());
      Serial.println("%");
      Serial.println(chargerState.get_status_str());
      break;
    }

    case utils::hash("format-fs"):
      Serial.println("clearing the whole file format");
      fileSystem::clear_internal_fs();
      break;

    default:
      Serial.print("unknown command: ");
      Serial.println(command);
      Serial.println("type h for available commands");
      break;
  }
}

String inputString = "";

void setup() {
  Serial.begin(115200);

  inputString.reserve(255);
}

void handleSerialEvents() {
  if (Serial.available()) {
    uint8_t lineRead = 0;
    uint8_t charRead = 0;

    inputString = "";

    // read available serial data
    do {
      // get the new byte:
      const char inChar = (char)Serial.read();
      // if the incoming character is a newline, finish parsing
      if (inChar == '\n') {
        // handle the command
        handleCommand(inputString);
        inputString = "";
        lineRead += 1;
        charRead = 0;
      } else {
        // add it to the inputString:
        inputString += inChar;
        charRead += 1;
      }
    } while (Serial.available() && lineRead < maxReadLinePerLoop &&
             charRead < maxLineLenght);

    inputString = "";
  }
}

}  // namespace serial
