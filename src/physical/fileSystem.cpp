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
std::map<std::string, uint32_t> _valueMap;

static bool isSetup = false;
void setup() {
  isSetup = true;
  InternalFS.begin();
}

void clear() { _valueMap.clear(); }

void load_initial_values() {
  if (!isSetup) {
    setup();
  }

  _valueMap.clear();

  file.open(FILENAME, FILE_O_READ);
  // file exist, good
  if (file and file.isOpen() and file.available()) {
    std::vector<char> vecRead(file.size());
    file.read(vecRead.data(), vecRead.size());

    bool isWord = true;
    bool isValue = false;
    std::string key = "";
    std::string value = "";
    // parser state machine
    for (const char c : vecRead) {
      if (c == '\n') {
        isWord = true;
        isValue = false;

        if (!key.empty() and !value.empty()) {
          _valueMap[key] = atoi(value.c_str());
        }
        key = "";
        value = "";
      } else if (c == ':') {
        isWord = false;
        isValue = true;
      } else if (isWord) {
        key += c;
      } else if (isValue) {
        value += c;
      }
    }

    if (!key.empty() and !value.empty()) {
      _valueMap[key] = atoi(value.c_str());
    }
    file.close();
  }
  // nothing to do
}

void write_state() {
  if (!isSetup) {
    setup();
  }

  // hardcore, format the entire file system
  // InternalFS.format();

  file.open(FILENAME, FILE_O_WRITE);
  file.truncate(0);  // clear the file
  file.close();

  delay(10);

  file.open(FILENAME, FILE_O_WRITE);
  for (const auto& keyval : _valueMap) {
    file.printf("%s:%d\n", keyval.first.c_str(), keyval.second);
  }
  file.close();
}

bool get_value(const std::string& key, uint32_t& value) {
  const auto& res = _valueMap.find(key);
  if (res != _valueMap.end()) {
    value = res->second;
    return true;
  }
  return false;
}

void set_value(const std::string& key, const uint32_t value) {
  _valueMap[key] = value;
}

}  // namespace fileSystem