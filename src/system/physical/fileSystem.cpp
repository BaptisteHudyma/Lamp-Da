#include "fileSystem.h"

#ifndef LMBD_SIMULATION
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#else
#include "simulator/mocks/Adafruit_LittleFS.h"
#include "simulator/mocks/InternalFileSystem.h"
#endif

#include <cassert>
#include <unordered_map>
#include <vector>

#include "src/system/utils/constants.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

namespace fileSystem {

static constexpr auto FILENAME = "/.lampda.par";

using namespace Adafruit_LittleFS_Namespace;

File file(InternalFS);

// the stored configurations
std::unordered_map<uint32_t, uint32_t> _valueMap;

struct keyValue
{
  uint32_t key;
  uint32_t value;
};
constexpr size_t sizeOfData = sizeof(keyValue);

// used to convert this struct to an array of bytes
union KeyValToByteArray
{
  uint8_t data[sizeOfData];
  keyValue kv;
};

static bool isSetup = false;
void setup()
{
  if (!InternalFS.begin())
  {
    lampda_print("Failed to start file system");
  }
  else
  {
    isSetup = true;
  }
}

void clear() { _valueMap.clear(); }

void clear_internal_fs()
{
  // hardcore, format the entire file system
  InternalFS.format();
}

bool load_initial_values()
{
  if (!isSetup)
  {
    setup();
  }

  // failure case: TODO: something ?
  if (!isSetup)
  {
    return false;
  }

  _valueMap.clear();

  // file exist, good
  if (file.open(FILENAME, FILE_O_READ) and file.isOpen() and file.available())
  {
    std::vector<char> vecRead(file.size());
    const int retVal = file.read(vecRead.data(), vecRead.size());
    if (retVal < 0)
    {
      // error case
      return false;
    }

    KeyValToByteArray converter;
    converter.kv.key = 0;
    converter.kv.value = 0;

    uint8_t cnt = 0; // when this reaches 8, a new word
    // parser state machine
    for (const char c: vecRead)
    {
      if (cnt >= sizeOfData)
      {
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
    if (cnt >= sizeOfData)
    {
      _valueMap[converter.kv.key] = converter.kv.value;
    }

    file.close();
    return true;
  }
  else
  {
    // no initial values, first boost maybe
  }
  return false;
}

void write_state()
{
  if (!isSetup)
  {
    setup();
  }

  // failure case: TODO: something ?
  if (!isSetup)
  {
    return;
  }

  file.open(FILENAME, FILE_O_WRITE);
  if (file)
  {
    file.truncate(0); // clear the file
    file.close();
  }
  else
  {
    // error. the file should have been opened
    lampda_print("file system error, reseting file format");

    // hardcore, format the entire file system
    InternalFS.format();
  }

  delay_ms(10);

  file.open(FILENAME, FILE_O_WRITE);
  if (file)
  {
    for (const auto& keyval: _valueMap)
    {
      KeyValToByteArray converter;
      converter.kv.key = keyval.first;
      converter.kv.value = keyval.second;
      // write the data converted to a byte array
      file.write(converter.data, sizeOfData);
    }
    file.close();
  }
  else
  {
    // error. the file should have been opened
    lampda_print("file creation failed, parameters wont be stored");
  }
}

bool doKeyExists(const uint32_t key) { return _valueMap.find(key) != _valueMap.end(); }

bool get_value(const uint32_t key, uint32_t& value)
{
#ifdef LMBD_SIMULATION
  fprintf(stderr, "fs: get_value %08x -> ", key);
#endif

  const auto& res = _valueMap.find(key);
  if (res != _valueMap.end())
  {
    value = res->second;

#ifdef LMBD_SIMULATION
    fprintf(stderr, "%08x\n", value);
#endif

    return true;
  }

#ifdef LMBD_SIMULATION
  fprintf(stderr, "not found\n");
#endif

  return false;
}

void set_value(const uint32_t key, const uint32_t value)
{
  _valueMap[key] = value;

#ifdef LMBD_SIMULATION
  fprintf(stderr, "fs: set_value %08x -> %08x\n", key, value);
#endif
}

uint32_t dropMatchingKeys(const uint32_t bitMatch, const uint32_t bitSelect)
{
  auto& c = _valueMap;

  // std::erase_if implementation
  //
  auto old_size = c.size();
  for (auto first = c.begin(), last = c.end(); first != last;)
  {
    uint32_t key = std::get<0>(*first);
    if ((key & bitSelect) == bitMatch)
    {
      first = c.erase(first);

#ifdef LMBD_SIMULATION
      fprintf(stderr, "fs: key dropped %08x (matches %08x)\n", key, bitMatch & bitSelect);
#endif
    }
    else
    {
      ++first;
    }
  }
  return old_size - c.size();
}

} // namespace fileSystem
