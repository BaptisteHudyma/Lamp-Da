#define HAL_FILESYSTEM_CPP

#include "src/system/hal/filesystem.h"

#include <cstdio>
#include <iostream>
#include <unistd.h>

namespace lampda {
namespace hal {
namespace filesystem {

// Do not care
class FileInternalTy
{
public:
  FileInternalTy() : file(nullptr), filemode {nullptr} {}

  FILE* file;
  char filename[256];
  const char* filemode;
};

extern bool setup() { return true; }

extern void shutdown() {}

void format_file_system() { std::cerr << "warning: InternalFS.format called\n" << std::endl; }

HAL_File::HAL_File() { mInternalFile = std::make_unique<FileInternalTy>(); }

HAL_File::~HAL_File()
{
  if (is_open())
    close();
}

bool HAL_File::open(const char* fname, const HAL_File::OpenType& mode)
{
  if (mInternalFile->file != nullptr)
  {
    fprintf(stderr, "fopen: opening two files?\n");
    mInternalFile->filemode = nullptr;
    fclose(mInternalFile->file);
  }

  switch (mode)
  {
    case OpenType::READ:
      mInternalFile->filemode = "r+";
      break;
    case OpenType::WRITE:
      mInternalFile->filemode = "w+";
      break;
    default:
      return false;
  }

  snprintf(mInternalFile->filename, sizeof(mInternalFile->filename), ".%s", fname);

  mInternalFile->file = fopen(mInternalFile->filename, mInternalFile->filemode);
  if (mInternalFile->file == nullptr)
  {
    fprintf(stderr, "fopen: failed with %s %s\n", mInternalFile->filename, mInternalFile->filemode);
  }
  return mInternalFile->file != nullptr;
}

bool HAL_File::is_open() const { return mInternalFile->file != nullptr; }

bool HAL_File::is_available() const { return mInternalFile->file != nullptr; }

void HAL_File::close()
{
  if (mInternalFile->file != nullptr)
  {
    fclose(mInternalFile->file);
    mInternalFile->file = nullptr;
    mInternalFile->filemode = nullptr;
  }
  else
  {
    fprintf(stderr, "error: closing a closed file!\n");
  }
}

size_t HAL_File::size() const
{
  if (mInternalFile->file != nullptr)
  {
    size_t pos = ftell(mInternalFile->file);
    fseek(mInternalFile->file, 0L, SEEK_END);
    size_t size = ftell(mInternalFile->file);
    fseek(mInternalFile->file, pos, SEEK_SET);
    return size;
  }

  fprintf(stderr, "error: measuring size on a closed file!\n");
  return 0;
}

size_t HAL_File::write(uint8_t* in, size_t sz)
{
  if (mInternalFile->file != nullptr)
  {
    // uncomment this to log all filesystem write
#if 0
      fprintf(stderr, "write: (%d) ", sz);
      for (size_t I = 0; I < sz; ++I)
      {
        fprintf(stderr, "%02x", in[I]);
      }
      fprintf(stderr, "\n");
#endif

    return fwrite(in, sizeof(uint8_t), sz, mInternalFile->file);
  }

  fprintf(stderr, "error: writing on a closed file!\n");
  return 0;
}

size_t HAL_File::read(uint8_t* out, size_t sz)
{
  if (mInternalFile->file != nullptr)
  {
    return fread(out, sizeof(uint8_t), sz, mInternalFile->file);
  }

  fprintf(stderr, "error: reading a closed file!\n");
  return 0;
}

bool HAL_File::seek(uint8_t sz)
{
  if (mInternalFile->file != nullptr)
  {
    if (mInternalFile->filemode != nullptr)
    {
      ::fseek(mInternalFile->file, sz, 0);
      return true;
    }
    else
    {
      fprintf(stderr, "error: invalid seek filename\n");
    }
  }
  else
  {
    fprintf(stderr, "error: seek closed file\n");
  }
  return false;
}

void HAL_File::truncate(uint8_t sz)
{
  if (mInternalFile->file != nullptr)
  {
    fclose(mInternalFile->file);
    mInternalFile->file = nullptr;

    if (mInternalFile->filemode != nullptr)
    {
      ::truncate(mInternalFile->filename, sz);
    }
    else
    {
      fprintf(stderr, "error: invalid truncate filename\n");
    }

    if (mInternalFile->filemode != nullptr)
    {
      mInternalFile->file = fopen(mInternalFile->filename, mInternalFile->filemode);
    }
    else
    {
      fprintf(stderr, "error: invalid truncate fopen\n");
    }
  }
  else
  {
    fprintf(stderr, "error: truncate closed file\n");
  }
}

// HAL_File::operator bool() const { return mInternalFile->file != nullptr; }

} // namespace filesystem
} // namespace hal
} // namespace lampda
