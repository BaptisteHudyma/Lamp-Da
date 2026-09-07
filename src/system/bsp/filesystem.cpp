#include "filesystem.h"

#include "src/system/hal/filesystem.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"

#include <map>
#include <vector>

namespace lampda {
namespace bsp {
namespace filesystem {

/// Store lamp general parameters
static constexpr const char* const FILENAME_USER = "/.lampda.par";
/// store lamp internal parameters, that should not be erased
static constexpr const char* const FILENAME_INTERNAL = "/.internal.par";

/// instance to avoid recreating objects
hal::filesystem::HAL_File paramFile;

size_t lastUserParameterSize = 0;
std::map<uint32_t, uint32_t> _userParametersValueMap;
std::map<uint32_t, uint32_t> _systemParametersValueMap;

/**
 * \brief Store a key and a value
 */
struct keyValue
{
  uint32_t key;   ///< key to match this value
  uint32_t value; ///< value associated with the key
};
constexpr size_t sizeOfData = sizeof(keyValue);

/**
 * \brief Used to convert a KeyValues to an array of bytes
 */
union KeyValToByteArray
{
  uint8_t data[sizeOfData]; ///< object as an array of bytes
  keyValue kv;              ///< original object
};

void clear()
{
  lastUserParameterSize = _userParametersValueMap.size();
  _userParametersValueMap.clear();
  // never clear system prameters
}

void clear_system_parameters() { _systemParametersValueMap.clear(); }

void shutdown() { hal::filesystem::shutdown(); }

void clear_internal_fs()
{
  hal::filesystem::format_file_system();
  hal::delay_ms(10);
}

namespace __internal {

/// Internal setup
bool setup() { return hal::filesystem::setup(); }

bool read_file_content(const char* fileName, std::map<uint32_t, uint32_t>& paramMap)
{
  if (not setup())
  {
    // failure case: TODO: something ?
    return false;
  }

  paramMap.clear();
  if (paramFile.open(fileName, hal::filesystem::HAL_File::OpenType::READ) and paramFile.is_open() and
      paramFile.is_available())
  {
    const auto fileSize = paramFile.size();
    if (fileSize <= 0)
      return false;
    if (not paramFile.seek(0))
      return false;

    std::vector<uint8_t> vecRead(fileSize);
    // THE FILESYSTEM CAN GET CORRUPTED, AND THE LINE BELOW WILL RUN FOREVER
    const int retVal = paramFile.read((uint8_t*)vecRead.data(), vecRead.size());

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

bool write_file(const char* filePath, const std::map<uint32_t, uint32_t>& paramMap, const bool shouldEraseFirst = false)
{
  if (not setup())
  {
    // failure case: TODO: something ?
    return false;
  }

  // check if it exists
  if (paramFile.open(filePath, hal::filesystem::HAL_File::OpenType::WRITE) and paramFile.is_open())
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
    bsp::lampda_print("file system error, reseting file format");

    // hardcore, format the entire file system
    hal::filesystem::format_file_system();
  }

  if (not paramFile.is_open())
    paramFile.open(filePath, hal::filesystem::HAL_File::OpenType::WRITE);
  if (paramFile.is_open())
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
    bsp::lampda_print("file creation failed, system parameters wont be stored");
    return false;
  }

  return true;
}

} // namespace __internal

namespace system {

bool doKeyExists(const uint32_t key) { return _systemParametersValueMap.find(key) != _systemParametersValueMap.end(); }

bool get_value(const uint32_t key, uint32_t& value)
{
  const auto& res = _systemParametersValueMap.find(key);
  if (res != _systemParametersValueMap.end())
  {
    value = res->second;

#ifdef LMBD_SIMULATION
    bsp::lampda_print("fsi: get_value %08x -> %08x", key, value);
#endif

    return true;
  }

#ifdef LMBD_SIMULATION
  bsp::lampda_print("fsi: get_value %08x -> not found", key);
#endif

  return false;
}

void set_value(const uint32_t key, const uint32_t value)
{
  _systemParametersValueMap[key] = value;

#ifdef LMBD_SIMULATION
  bsp::lampda_print("fsi: set_value %08x -> %08x", key, value);
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
      bsp::lampda_print("fsi: key dropped %08x (matches %08x)", key, bitMatch & bitSelect);
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
  // write internal parameters
  const bool systemParameterWriteSuccess = __internal::write_file(FILENAME_INTERNAL, _systemParametersValueMap);
  if (not systemParameterWriteSuccess)
  {
    // TODO: handle error
    bsp::lampda_print("could not save system parameters");
  }
}

bool load_from_file() { return __internal::read_file_content(FILENAME_INTERNAL, _systemParametersValueMap); }

} // namespace system

namespace user {

bool doKeyExists(const uint32_t key) { return _userParametersValueMap.find(key) != _userParametersValueMap.end(); }

bool get_value(const uint32_t key, uint32_t& value)
{
  const auto& res = _userParametersValueMap.find(key);
  if (res != _userParametersValueMap.end())
  {
    value = res->second;

#ifdef LMBD_SIMULATION
    bsp::lampda_print("fsu: get_value %08x -> %08x", key, value);
#endif

    return true;
  }

#ifdef LMBD_SIMULATION
  bsp::lampda_print("fsu: get_value %08x -> not found", key);
#endif

  return false;
}

void set_value(const uint32_t key, const uint32_t value)
{
  _userParametersValueMap[key] = value;

#ifdef LMBD_SIMULATION
  bsp::lampda_print("fsu: set_value %08x -> %08x", key, value);
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
      bsp::lampda_print("fsu: key dropped %08x (matches %08x)", key, bitMatch & bitSelect);
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
  // user first
  const bool shouldEraseFile = lastUserParameterSize > _userParametersValueMap.size();
  const bool userParameterWriteSuccess =
          __internal::write_file(FILENAME_USER, _userParametersValueMap, shouldEraseFile);
  if (not userParameterWriteSuccess)
  {
    // TODO: handle error
    bsp::lampda_print("could not save user parameters");
  }
}

bool load_from_file() { return __internal::read_file_content(FILENAME_USER, _userParametersValueMap); }

} // namespace user

} // namespace filesystem
} // namespace bsp
} // namespace lampda
