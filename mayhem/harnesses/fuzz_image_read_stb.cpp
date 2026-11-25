// Image decoding through vpImageIo with the embedded stb_image backend (JPEG/PNG/BMP and the
// other formats stb sniffs), plus the buffer API vpImageIo::readPNGfromMem.
// [0] low 3 bits select the file extension vpImageIo::read dispatches on, bit 3 selects the file API
// or the buffer API (readPNGfromMem); the rest is the image.
#include "fuzz_common.h"
#include <vector>
#include <visp3/core/vpImage.h>
#include <visp3/io/vpImageIo.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 1)
    return -1;
  static const char *kExt[] = { ".jpg", ".png", ".bmp", ".tga", ".gif", ".psd", ".hdr", ".pic" };
  const char *ext = kExt[data[0] % 8];
  const bool mem = data[0] & 8;
  const uint8_t *img = data + 1;
  size_t n = size - 1;
  const int be = vpImageIo::IO_STB_IMAGE_BACKEND;
  if (mem) {
    std::vector<unsigned char> buf(img, img + n);
    // Only the vpRGBa overload: the grayscale readPNGfromMemStb() asserts comp == 1
    // (vpImageIoStb.cpp:155) after stbi_load_from_memory, which fails for every undecodable or
    // non-grayscale buffer (comp is the file's channel count, 0 on error), so it aborts on nearly
    // any input and would mask everything else. The RGBa overload checks comp and throws.
    {
      vpImage<vpRGBa> I;
      FUZZ_TRY(vpImageIo::readPNGfromMem(buf, I, be));
    }
    return 0;
  }
  std::string path = fuzz_write_tmp(img, n, ext);
  {
    vpImage<unsigned char> I;
    FUZZ_TRY(vpImageIo::read(I, path, be));
  }
  {
    vpImage<vpRGBa> I;
    FUZZ_TRY(vpImageIo::read(I, path, be));
  }
  unlink(path.c_str());
  return 0;
}
