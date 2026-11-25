// ViSP's own portable-image parsers: vpImageIo::readPGM / readPPM / readPFM / readPFM_HDR
// (modules/io/src/image/private/vpImageIoPortable.cpp).
#include "fuzz_common.h"
#include <visp3/core/vpImage.h>
#include <visp3/core/vpRGBf.h>
#include <visp3/io/vpImageIo.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".pnm");
  { vpImage<unsigned char> I; FUZZ_TRY(vpImageIo::readPGM(I, path)); }
  { vpImage<vpRGBa> I; FUZZ_TRY(vpImageIo::readPGM(I, path)); }
  { vpImage<unsigned char> I; FUZZ_TRY(vpImageIo::readPPM(I, path)); }
  { vpImage<vpRGBa> I; FUZZ_TRY(vpImageIo::readPPM(I, path)); }
  { vpImage<float> I; FUZZ_TRY(vpImageIo::readPFM(I, path)); }
  { vpImage<float> I; FUZZ_TRY(vpImageIo::readPFM_HDR(I, path)); }
  { vpImage<vpRGBf> I; FUZZ_TRY(vpImageIo::readPFM_HDR(I, path)); }
  unlink(path.c_str());
  return 0;
}
