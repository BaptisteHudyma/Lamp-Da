#include "fileSystem.h"

#ifndef LMBD_SIMULATION
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#else
#include "simulator/mocks/Adafruit_LittleFS.h"
#include "simulator/mocks/InternalFileSystem.h"
#endif

#include <cassert>
#include <map>
#include <vector>

#include "src/system/utils/constants.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

namespace fileSystem {

static constexpr auto FILENAME_USER = "/.lampda.par";
// store lamp internal parameters, that should not be erased
static constexpr auto FILENAME_INTERNAL = "/.internal.par";

using namespace Adafruit_LittleFS_Namespace;
File paramFile(InternalFS); // instance to avoid recreating objects

size_t lastUserParameterSize = 0;
std::map<uint32_t, uint32_t> _userParametersValueMap;
std::map<uint32_t, uint32_t> _systemParametersValueMap;

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

void clear()
{
  lastUserParameterSize = _userParametersValueMap.size();
  _userParametersValueMap.clear();
  // never clear lamp prameters
}

void clear_internal_fs()
{
  // hardcore, format the entire file system
  InternalFS.format();
}

namespace __internal {

bool read_file_content(const char* fileName, std::map<uint32_t, uint32_t>& paramMap)
{
  paramMap.clear();
  if (paramFile.open(fileName, FILE_O_READ) and paramFile.isOpen() and paramFile.available())
  {
    std::vector<char> vecRead(paramFile.size());
    const int retVal = paramFile.read(vecRead.data(), vecRead.size());
    if (retVal < 0)
    {
      // error case
      return false;
    }

    KeyValToByteArray converter;
    converter.kv.key = 0;
    converter.kv.value = 0;

    bool hasDuplicates = false;

    uint8_t cnt = 0; // when this reaches 8, a new word
    // parser state machine
    for (const char c: vecRead)
    {
      if (cnt >= sizeOfData)
      {
        // only modify the first data of the list
        if (paramMap.find(converter.kv.key) == paramMap.end())
        {
          paramMap[converter.kv.key] = converter.kv.value;
        }
        else
        {
          hasDuplicates = true;
        }

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
      paramMap[converter.kv.key] = converter.kv.value;
    }

    // erase file content in case of duplicates
    if (hasDuplicates)
      paramFile.truncate(0);

    paramFile.close();
    return true;
  }
  return false;
}

bool write_file(const char* filePath, std::map<uint32_t, uint32_t>& paramMap, const bool shouldEraseFirst = false)
{
  // check if it exists
  if (paramFile.open(filePath, FILE_O_WRITE) and paramFile.isOpen())
  {
    if (not shouldEraseFirst)
      paramFile.seek(0); // return to the begining of the file
    else
    {
      // erase file content
      paramFile.truncate(0);
      paramFile.close();
    }
  }
  else
  {
    // error. the file should have been opened
    lampda_print("file system error, reseting file format");

    // hardcore, format the entire file system
    InternalFS.format();
    delay_ms(10);
  }

  if (not paramFile.isOpen())
    paramFile.open(filePath, FILE_O_WRITE);
  if (paramFile.isOpen())
  {
    for (const auto& keyval: paramMap)
    {
      KeyValToByteArray converter;
      converter.kv.key = keyval.first;
      converter.kv.value = keyval.second;
      // write the data converted to a byte array
      paramFile.write(converter.data, sizeOfData);
    }
    paramFile.close();
  }
  else
  {
    // error. the file should have been opened
    lampda_print("file creation failed, system parameters wont be stored");
    return false;
  }

  return true;
}

} // namespace __internal

namespace system {

bool doKeyExists(const uint32_t key) { return _systemParametersValueMap.find(key) != _systemParametersValueMap.end(); }

bool get_value(const uint32_t key, uint32_t& value)
{
#ifdef LMBD_SIMULATION
  fprintf(stderr, "fsi: get_value %08x -> ", key);
#endif

  const auto& res = _systemParametersValueMap.find(key);
  if (res != _systemParametersValueMap.end())
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
  _systemParametersValueMap[key] = value;

#ifdef LMBD_SIMULATION
  fprintf(stderr, "fsi: set_value %08x -> %08x\n", key, value);
#endif
}

uint32_t dropMatchingKeys(const uint32_t bitMatch, const uint32_t bitSelect)
{
  auto& c = _systemParametersValueMap;

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
      fprintf(stderr, "fsi: key dropped %08x (matches %08x)\n", key, bitMatch & bitSelect);
#endif
    }
    else
    {
      ++first;
    }
  }
  return old_size - c.size();
}

void write_to_file()
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

  // write internal parameters
  const bool systemParameterWriteSuccess = __internal::write_file(FILENAME_INTERNAL, _systemParametersValueMap);
  if (not systemParameterWriteSuccess)
  {
    // TODO: handle error
    lampda_print("could not save system parameters");
  }
}

bool load_from_file()
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

  return __internal::read_file_content(FILENAME_INTERNAL, _systemParametersValueMap);
}

} // namespace system

namespace user {

bool doKeyExists(const uint32_t key) { return _userParametersValueMap.find(key) != _userParametersValueMap.end(); }

bool get_value(const uint32_t key, uint32_t& value)
{
#ifdef LMBD_SIMULATION
  fprintf(stderr, "fsu: get_value %08x -> ", key);
#endif

  const auto& res = _userParametersValueMap.find(key);
  if (res != _userParametersValueMap.end())
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
  _userParametersValueMap[key] = value;

#ifdef LMBD_SIMULATION
  fprintf(stderr, "fsu: set_value %08x -> %08x\n", key, value);
#endif
}

uint32_t dropMatchingKeys(const uint32_t bitMatch, const uint32_t bitSelect)
{
  auto& c = _userParametersValueMap;

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
      fprintf(stderr, "fsu: key dropped %08x (matches %08x)\n", key, bitMatch & bitSelect);
#endif
    }
    else
    {
      ++first;
    }
  }
  return old_size - c.size();
}

void write_to_file()
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

  // user first
  const bool shouldEraseFile = lastUserParameterSize > _userParametersValueMap.size();
  const bool userParameterWriteSuccess =
          __internal::write_file(FILENAME_USER, _userParametersValueMap, shouldEraseFile);
  if (not userParameterWriteSuccess)
  {
    // TODO: handle error
    lampda_print("could not save user parameters");
  }
}

bool load_from_file()
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

  return __internal::read_file_content(FILENAME_USER, _userParametersValueMap);
}

} // namespace user

} // namespace fileSystem
