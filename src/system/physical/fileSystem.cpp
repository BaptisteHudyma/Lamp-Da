#include "fileSystem.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include <map>
#include <vector>

#include "../utils/constants.h"

namespace fileSystem {

constexpr auto FILENAME = "/.lampda.par";

using namespace Adafruit_LittleFS_Namespace;

File file(InternalFS);

// the stored configurations
std::map<uint32_t, uint32_t> _valueMap;

// used to convert two uint32 to 8 uint8
union KeyValToByteArray {
  uint8_t data[8];
  uint32_t d[2];
};

static bool isSetup = false;
void setup() {
  if (!InternalFS.begin()) {
    Serial.println("Failed to start file system");
  } else {
    isSetup = true;
  }
}

void clear() { _valueMap.clear(); }

void load_initial_values() {
  if (!isSetup) {
    setup();
  }

  // failure case: TODO: something ?
  if (!isSetup) {
    return;
  }

  _valueMap.clear();

  // file exist, good
  if (file.open(FILENAME, FILE_O_READ) and file.isOpen() and file.available()) {
    std::vector<char> vecRead(file.size());
    const int retVal = file.read(vecRead.data(), vecRead.size());
    if (retVal < 0) {
      // error case
      return;
    }

    KeyValToByteArray converter;
    converter.d[0] = 0;
    converter.d[1] = 0;

    uint8_t cnt = 0;  // when this reaches 8, a new word
    // parser state machine
    for (const char c : vecRead) {
      if (cnt >= 8) {
        _valueMap[converter.d[0]] = converter.d[1];

        // reset
        converter.d[0] = 0;
        converter.d[1] = 0;
        cnt = 0;
      }

      converter.data[cnt] = c;
      cnt++;
    }

    // last word !
    if (cnt >= 8) {
      _valueMap[converter.d[0]] = converter.d[1];
    }

    file.close();
  } else {
    // no initial values, first boost maybe
  }
}

void write_state() {
  if (!isSetup) {
    setup();
  }

  // failure case: TODO: something ?
  if (!isSetup) {
    return;
  }

  file.open(FILENAME, FILE_O_WRITE);
  if (file) {
    file.truncate(0);  // clear the file
    file.close();
  } else {
    // error. the file should have been opened
    Serial.println("file system error, reseting file format");

    // hardcore, format the entire file system
    InternalFS.format();
  }

  delay(10);

  file.open(FILENAME, FILE_O_WRITE);
  if (file) {
    for (const auto& keyval : _valueMap) {
      KeyValToByteArray converter;
      converter.d[0] = keyval.first;
      converter.d[1] = keyval.second;
      file.write(converter.data,
                 8  // sizeof(converter.data)
      );
    }
    file.close();
  } else {
    // error. the file should have been opened
    Serial.println("file creation failed, parameters wont be stored");
  }
}

bool get_value(const uint32_t key, uint32_t& value) {
  const auto& res = _valueMap.find(key);
  if (res != _valueMap.end()) {
    value = res->second;
    return true;
  }
  return false;
}

void set_value(const uint32_t key, const uint32_t value) {
  _valueMap[key] = value;
}

}  // namespace fileSystem