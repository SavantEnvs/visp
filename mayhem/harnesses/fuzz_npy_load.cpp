// NumPy .npy / .npz readers: visp::cnpy::npy_load and npz_load (vpIoTools_npy.cpp: npy header
// parse, zip local/central headers, deflate via the bundled miniz).
#include "fuzz_common.h"
#include <visp3/core/vpIoTools.h>

static size_t touch(const visp::cnpy::NpyArray &a)
{
  size_t s = 0;
  const char *p = a.data<char>();
  size_t n = a.num_bytes();
  for (size_t i = 0; p && i < n; ++i)
    s += (unsigned char)p[i];
  return s;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  volatile size_t sink = 0;
  std::string npy = fuzz_write_tmp(data, size, ".npy");
  FUZZ_TRY(sink += touch(visp::cnpy::npy_load(npy)));
  unlink(npy.c_str());
  std::string npz = fuzz_write_tmp(data, size, ".npz");
  FUZZ_TRY({
    visp::cnpy::npz_t arrays = visp::cnpy::npz_load(npz);
    for (const auto &kv : arrays)
      sink += touch(kv.second);
  });
  FUZZ_TRY(sink += touch(visp::cnpy::npz_load(npz, "arr_0")));
  unlink(npz.c_str());
  (void)sink;
  return 0;
}
