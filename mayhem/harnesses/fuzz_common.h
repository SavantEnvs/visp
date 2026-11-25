// Shared helpers for the ViSP fuzz harnesses (mayhem layer).
// Most ViSP parsers only take a file name and dispatch on its extension, so the
// harnesses stage the input in a per-process scratch file under $TMPDIR.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <exception>

#include <visp3/core/vpConfig.h>
#include <visp3/core/vpException.h>

#if defined(ENABLE_VISP_NAMESPACE)
using namespace VISP_NAMESPACE_NAME;
#endif

// Writes data to "$TMPDIR/visp_fuzz_<pid>_<tag><ext>" (overwritten every input) and returns its path.
static inline std::string fuzz_write_tmp(const uint8_t *data, size_t size, const std::string &ext,
                                         const std::string &tag = "in")
{
  const char *tmp = getenv("TMPDIR");
  std::string dir = (tmp && *tmp) ? tmp : "/tmp";
  std::string path = dir + "/visp_fuzz_" + std::to_string(getpid()) + "_" + tag + ext;
  FILE *f = fopen(path.c_str(), "wb");
  if (!f)
    abort();
  if (size && fwrite(data, 1, size, f) != size)
    abort();
  fclose(f);
  return path;
}

#define FUZZ_TRY(...)                                                                                                \
  do {                                                                                                               \
    try {                                                                                                            \
      __VA_ARGS__;                                                                                                       \
    }                                                                                                                \
    catch (const vpException &) {                                                                                    \
    }                                                                                                                \
    catch (const std::exception &) {                                                                                 \
    }                                                                                                                \
  } while (0)
