// CAO 3D model parser: vpMbGenericTracker::loadModel(<file>.cao) (vpMbTracker::loadCAOModel —
// points, lines, faces, cylinders, circles) feeding the edge and depth trackers' face setup.
// The input's recursive "load(...)" includes are resolved relative to the scratch file.
#include "fuzz_common.h"
#include <visp3/mbt/vpMbGenericTracker.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".cao");
  FUZZ_TRY({
    vpMbGenericTracker tracker(1, vpMbGenericTracker::EDGE_TRACKER | vpMbGenericTracker::DEPTH_DENSE_TRACKER |
                                      vpMbGenericTracker::DEPTH_NORMAL_TRACKER);
    tracker.loadModel(path, false);
  });
  unlink(path.c_str());
  return 0;
}
