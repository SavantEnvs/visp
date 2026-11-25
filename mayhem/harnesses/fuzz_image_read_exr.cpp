// OpenEXR decoding through vpImageIo::readEXR (embedded TinyEXR backend).
#include "fuzz_common.h"
#include <visp3/core/vpImage.h>
#include <visp3/core/vpRGBf.h>
#include <visp3/io/vpImageIo.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".exr");
  { vpImage<float> I; FUZZ_TRY(vpImageIo::readEXR(I, path)); }
  { vpImage<vpRGBf> I; FUZZ_TRY(vpImageIo::readEXR(I, path)); }
  unlink(path.c_str());
  return 0;
}
