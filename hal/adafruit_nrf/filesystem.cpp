#include "src/system/hal/filesystem.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

namespace lampda {
namespace hal {
namespace filesystem {

using namespace Adafruit_LittleFS_Namespace;

class FileInternalTy : public File
{
  /// Inheritate constructors
  using File::File;
};

namespace __private {

/// Keep track of init state
static bool isInternalFsStarted = false;

bool setup_shared_filesystem_instance()
{
  if (not isInternalFsStarted)
  {
    if (InternalFS.begin())
    {
      isInternalFsStarted = true;
    }
    else
    {
      // TODO: error
      return false;
    }
  }
  return true;
}

void shutdown_shared_filesystem_instance()
{
  if (isInternalFsStarted)
  {
    InternalFS.end();
    isInternalFsStarted = false;
  }
}

} // namespace __private

bool setup() { return __private::setup_shared_filesystem_instance(); }

void shutdown() { __private::shutdown_shared_filesystem_instance(); }

void format_file_system()
{
  __private::setup_shared_filesystem_instance();
  InternalFS.format();
}

/**
 *
 *
 */

bool is_setup() { return __private::isInternalFsStarted; }

HAL_File::HAL_File()
{
  // Shared instance, if not already started
  __private::setup_shared_filesystem_instance();

  // build LittleFS system
  mInternalFile = std::unique_ptr<FileInternalTy>(new FileInternalTy(InternalFS));
}

HAL_File::~HAL_File()
{
  if (is_open())
    close();
}

bool HAL_File::open(const char* fname, const HAL_File::OpenType& mode)
{
  if (not is_setup() or not mInternalFile)
    return false;

  switch (mode)
  {
    case OpenType::READ:
      return mInternalFile->open(fname, FILE_O_READ);
    case OpenType::WRITE:
      return mInternalFile->open(fname, FILE_O_WRITE);
    default:
      return false;
  }
  return false;
}

bool HAL_File::is_open() const
{
  if (not is_setup() or not mInternalFile)
    return false;
  return mInternalFile->isOpen();
}

bool HAL_File::is_available() const
{
  if (not is_setup() or not mInternalFile)
    return false;
  return mInternalFile->available();
}

void HAL_File::close()
{
  if (not is_setup() or not mInternalFile)
    return;
  mInternalFile->close();
}

size_t HAL_File::size() const
{
  if (not is_setup() or not mInternalFile)
    return 0;
  return mInternalFile->size();
}

size_t HAL_File::write(uint8_t* in, size_t sz)
{
  if (not is_setup() or not mInternalFile)
    return false;
  return mInternalFile->write(in, sz);
}

size_t HAL_File::read(uint8_t* out, size_t sz)
{
  if (not is_setup() or not mInternalFile)
    return 0;
  return mInternalFile->read(out, sz);
}

bool HAL_File::seek(uint8_t sz)
{
  if (not is_setup() or not mInternalFile)
    return false;
  return mInternalFile->seek(sz);
}

void HAL_File::truncate(uint8_t sz)
{
  if (not is_setup() or not mInternalFile)
    return;
  mInternalFile->truncate(sz);
}

} // namespace filesystem
} // namespace hal
} // namespace lampda
