#include "fileSystem.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include <cassert>
#include <map>
#include <vector>

#include "src/system/utils/constants.h"

namespace fileSystem {

constexpr auto FILENAME = "/.lampda.par";

using namespace Adafruit_LittleFS_Namespace;

File file(InternalFS);

// the stored configurations
std::map<uint32_t, uint32_t> _valueMap;

struct keyValue {
  uint32_t key;
  uint32_t value;
};
constexpr size_t sizeOfData = sizeof(keyValue);

// used to convert this struct to an array of bytes
union KeyValToByteArray {
  uint8_t data[sizeOfData];
  keyValue kv;
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

void clear_internal_fs() {
  // hardcore, format the entire file system
  InternalFS.format();
}

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
    converter.kv.key = 0;
    converter.kv.value = 0;

    uint8_t cnt = 0;  // when this reaches 8, a new word
    // parser state machine
    for (const char c : vecRead) {
      if (cnt >= sizeOfData) {
        _valueMap[converter.kv.key] = converter.kv.value;

        // reset
        converter.kv.key = 0;
        converter.kv.value = 0;
        cnt = 0;
      }

      converter.data[cnt] = c;
      cnt++;
    }

    // last word !
    if (cnt >= sizeOfData) {
      _valueMap[converter.kv.key] = converter.kv.value;
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
      converter.kv.key = keyval.first;
      converter.kv.value = keyval.second;
      // write the data converted to a byte array
      file.write(converter.data, sizeOfData);
    }
    file.close();
  } else {
    // error. the file should have been opened
    Serial.println("file creation failed, parameters wont be stored");
  }
}

bool doKeyExists(const uint32_t key) {
  return _valueMap.find(key) != _valueMap.end();
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
