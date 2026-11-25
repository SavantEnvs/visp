// Model-based tracker XML settings: vpMbtXmlGenericParser::parse (pugixml; camera, ecm, klt,
// depth, lod, face, projection-error nodes) and vpMbGenericTracker::loadConfigFile(<file>.xml).
#include "fuzz_common.h"
#include <visp3/mbt/vpMbGenericTracker.h>
#include <visp3/mbt/vpMbtXmlGenericParser.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".xml");
  FUZZ_TRY({
    vpMbtXmlGenericParser p(vpMbtXmlGenericParser::EDGE_PARSER | vpMbtXmlGenericParser::KLT_PARSER |
                            vpMbtXmlGenericParser::DEPTH_NORMAL_PARSER |
                            vpMbtXmlGenericParser::DEPTH_DENSE_PARSER | vpMbtXmlGenericParser::PROJECTION_ERROR_PARSER);
    p.setVerbose(false);
    p.parse(path);
  });
  FUZZ_TRY({
    vpMbGenericTracker tracker(1, vpMbGenericTracker::EDGE_TRACKER | vpMbGenericTracker::DEPTH_DENSE_TRACKER |
                                      vpMbGenericTracker::DEPTH_NORMAL_TRACKER);
    tracker.loadConfigFile(path, false);
  });
  unlink(path.c_str());
  return 0;
}
