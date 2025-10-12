#ifndef FAKE_LITTLEFS_H
#define FAKE_LITTLEFS_H

#include <cstdio>
#include <unistd.h>

namespace Adafruit_LittleFS_Namespace {
static constexpr uint8_t _placeholder_littlefs = 0;

} // namespace Adafruit_LittleFS_Namespace

struct InternalFSTy
{
  bool begin()
  {
    // fprintf(stderr, "info: InternalFS.begin called\n");
    return true;
  }

  void format() { fprintf(stderr, "warning: InternalFS.format called\n"); }
};

InternalFSTy InternalFS {};

#define FILE_O_READ  "r+"
#define FILE_O_WRITE "w+"

struct File
{
  File(InternalFSTy& InternalFS) : file {nullptr}, filemode {nullptr}, InternalFS {InternalFS} {}

  bool open(const char* fname, const char* mode)
  {
    // fprintf(stderr, "fopen: %s %s\n", fname, mode);

    if (file != nullptr)
    {
      fprintf(stderr, "fopen: opening two files?\n");
      filemode = nullptr;
      fclose(file);
    }

    snprintf(filename, sizeof(filename), ".%s", fname);
    filemode = mode;
    file = fopen(filename, mode);
    if (file == nullptr)
    {
      fprintf(stderr, "fopen: failed with %s %s\n", filename, mode);
    }
    return file != nullptr;
  }

  bool isOpen() { return file != nullptr; }

  bool available() { return file != nullptr; }

  void close()
  {
    if (file != nullptr)
    {
      fclose(file);
      file = nullptr;
      filemode = nullptr;
    }
    else
    {
      fprintf(stderr, "error: closing a closed file!\n");
    }
  }

  size_t size()
  {
    if (file != nullptr)
    {
      size_t pos = ftell(file);
      fseek(file, 0L, SEEK_END);
      size_t size = ftell(file);
      fseek(file, pos, SEEK_SET);
      return size;
    }

    fprintf(stderr, "error: measuring size on a closed file!\n");
    return 0;
  }

  size_t write(uint8_t* in, size_t sz)
  {
    if (file != nullptr)
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

      return fwrite(in, sizeof(uint8_t), sz, file);
    }

    fprintf(stderr, "error: writing on a closed file!\n");
    return 0;
  }

  size_t read(char* out, size_t sz)
  {
    if (file != nullptr)
    {
      return fread(out, sizeof(char), sz, file);
    }

    fprintf(stderr, "error: reading a closed file!\n");
    return 0;
  }

  void seek(uint8_t sz)
  {
    if (file != nullptr)
    {
      if (filemode != nullptr)
      {
        ::fseek(file, sz, 0);
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
  }

  void truncate(uint8_t sz)
  {
    if (file != nullptr)
    {
      fclose(file);
      file = nullptr;

      if (filemode != nullptr)
      {
        ::truncate(filename, sz);
      }
      else
      {
        fprintf(stderr, "error: invalid truncate filename\n");
      }

      if (filemode != nullptr)
      {
        file = fopen(filename, filemode);
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

  operator bool() const { return file != nullptr; }

  FILE* file;
  char filename[256];
  const char* filemode;
  InternalFSTy& InternalFS;
};

#endif
