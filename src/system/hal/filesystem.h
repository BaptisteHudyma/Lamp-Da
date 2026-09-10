/*! \file hal/filesystem.h
    \brief Interface for the physical components of the file system.
*/

#ifndef HAM_FILESYSTEM_HPP
#define HAM_FILESYSTEM_HPP

#include <cstddef>
#include <cstdint>

#include <memory>

namespace lampda {
namespace hal {
/// Handle the interaction with the file system.
namespace filesystem {

/// Internal class to a file
class FileInternalTy;

/// Global filesystem shutdown
extern void shutdown();

/// Format/erase the whole file system.
extern void format_file_system();

/// Handle class for a file
struct HAL_File
{
  HAL_File();
  ~HAL_File();

  enum class OpenType
  {
    READ,
    WRITE,
  };

  bool open(const char* fname, const OpenType& mode);

  bool is_open() const;

  bool is_available() const;

  void close();

  size_t size() const;

  size_t write(const uint8_t* const in, size_t sz);

  size_t read(uint8_t* out, size_t sz);

  bool seek(uint8_t sz);

  void truncate(uint8_t sz);

  // operator bool() const;

private:
  std::unique_ptr<FileInternalTy> mInternalFile;
};

} // namespace filesystem
} // namespace hal
} // namespace lampda

#endif
