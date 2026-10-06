#include "src/system/hal/filesystem.h"

#include <InternalFileSystem.h>
#include <cstddef>

#include <etl/map.h>

#include "src/system/bsp/text_out.h"

namespace lampda {
namespace hal {
namespace filesystem {

class FileInternalTy : public Adafruit_LittleFS_Namespace::File
{
  /// Inheritate constructors
  using File::File;

public:
  size_t uniqueIndex;
};

namespace __private {

/// Keep track of init state
bool isInternalFsStarted = false;

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

bool is_setup() { return __private::isInternalFsStarted; }

} // namespace __private

void shutdown() { __private::shutdown_shared_filesystem_instance(); }

void format_file_system()
{
  // setup the filesystem if needed
  __private::setup_shared_filesystem_instance();

  InternalFS.format();
}

bool delete_file(const char* fname) { return InternalFS.remove(fname); }

/**
 *
 *
 */

static constexpr size_t maxFiles = 8;
static etl::map<size_t, size_t, maxFiles> usedIndicesMap;
static FileInternalTy static_files[] = {FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS),
                                        FileInternalTy(InternalFS)};

FileInternalTy* affect_next_free_file()
{
  for (size_t i = 0; i < maxFiles; i++)
  {
    if (not usedIndicesMap.contains(i))
    {
      usedIndicesMap.insert({i, i});
      static_files[i].uniqueIndex = i;
      return &static_files[i];
    }
  }
  return nullptr;
}

HAL_File::HAL_File()
{
  FileInternalTy* fileRef = affect_next_free_file();
  if (fileRef != nullptr)
    mInternalFile = {std::shared_ptr<FileInternalTy> {}, fileRef};

  // Do not start here, the filesystem may not exist yet
}

HAL_File::~HAL_File()
{
  if (mInternalFile)
    usedIndicesMap.erase(mInternalFile->uniqueIndex);
}

bool HAL_File::open(const char* fname, const HAL_File::OpenType& mode)
{
  // setup the filesystem !
  __private::setup_shared_filesystem_instance();

  if (not __private::is_setup() or not mInternalFile)
    return false;

  switch (mode)
  {
    case OpenType::READ:
      return mInternalFile->open(fname, Adafruit_LittleFS_Namespace::FILE_O_READ);
    case OpenType::WRITE:
      return mInternalFile->open(fname, Adafruit_LittleFS_Namespace::FILE_O_WRITE);
    default:
      return false;
  }
  return false;
}

bool HAL_File::is_open() const
{
  if (not __private::is_setup() or not mInternalFile)
    return false;
  return mInternalFile->isOpen();
}

bool HAL_File::is_available() const
{
  if (not __private::is_setup() or not mInternalFile)
    return false;
  return mInternalFile->available();
}

void HAL_File::close()
{
  if (not __private::is_setup() or not mInternalFile)
    return;
  mInternalFile->close();
}

size_t HAL_File::size() const
{
  if (not __private::is_setup() or not mInternalFile)
    return 0;
  return mInternalFile->size();
}

size_t HAL_File::write(const uint8_t* const in, size_t sz)
{
  if (not __private::is_setup() or not mInternalFile)
    return false;
  return mInternalFile->write(in, sz);
}

size_t HAL_File::read(uint8_t* out, size_t sz)
{
  if (not __private::is_setup() or not mInternalFile)
    return 0;
  return mInternalFile->read(out, sz);
}

bool HAL_File::seek(uint8_t sz)
{
  if (not __private::is_setup() or not mInternalFile)
    return false;
  return mInternalFile->seek(sz);
}

void HAL_File::truncate(uint8_t sz)
{
  if (not __private::is_setup() or not mInternalFile)
    return;
  mInternalFile->truncate(sz);
}

} // namespace filesystem
} // namespace hal
} // namespace lampda
