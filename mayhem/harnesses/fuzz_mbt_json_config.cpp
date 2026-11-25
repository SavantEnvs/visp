// Model-based tracker JSON settings: vpMbGenericTracker::loadConfigFile(<file>.json) ->
// loadConfigFileJSON + the nlohmann from_json converters for vpMe, vpCameraParameters,
// vpHomogeneousMatrix, KLT/depth settings and the per-camera tracker type.
#include "fuzz_common.h"
#include <visp3/mbt/vpMbGenericTracker.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#if defined(VISP_HAVE_NLOHMANN_JSON)
  std::string path = fuzz_write_tmp(data, size, ".json");
  FUZZ_TRY({
    vpMbGenericTracker tracker(1, vpMbGenericTracker::EDGE_TRACKER | vpMbGenericTracker::DEPTH_DENSE_TRACKER |
                                      vpMbGenericTracker::DEPTH_NORMAL_TRACKER);
    tracker.loadConfigFile(path, false);
  });
  unlink(path.c_str());
#else
#error "mbt-json-config needs ViSP built with nlohmann_json (USE_NLOHMANN_JSON=ON)"
#endif
  return 0;
}
