// Key/value configuration files: vpIoTools::loadConfigFile + every readConfigVar overload.
// loadConfigFile replaces vpIoTools' static parameter table, so no state carries across inputs.
#include "fuzz_common.h"
#include <visp3/core/vpColor.h>
#include <visp3/core/vpIoTools.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".cfg");
  FUZZ_TRY({
    if (vpIoTools::loadConfigFile(path)) {
      static const char *kNames[] = { "a", "b", "lambda", "color", "matrix", "x", "use2D", "n" };
      for (const char *name : kNames) {
        float f; double d; int i; unsigned int u; bool b; std::string s; vpColor c; vpArray2D<double> A;
        FUZZ_TRY(vpIoTools::readConfigVar(name, f));
        FUZZ_TRY(vpIoTools::readConfigVar(name, d));
        FUZZ_TRY(vpIoTools::readConfigVar(name, i));
        FUZZ_TRY(vpIoTools::readConfigVar(name, u));
        FUZZ_TRY(vpIoTools::readConfigVar(name, b));
        FUZZ_TRY(vpIoTools::readConfigVar(name, s));
        FUZZ_TRY(vpIoTools::readConfigVar(name, c));
        FUZZ_TRY(vpIoTools::readConfigVar(name, A));
        FUZZ_TRY(vpIoTools::readConfigVar(name, A, 3, 3));
      }
    }
  });
  unlink(path.c_str());
  return 0;
}
