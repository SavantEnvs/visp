// Core XML parsers (pugixml): vpXmlParserCamera, vpXmlParserHomogeneousMatrix and
// vpXmlParserRectOriented, all on the same document.
#include "fuzz_common.h"
#include <visp3/core/vpCameraParameters.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/core/vpXmlParserCamera.h>
#include <visp3/core/vpXmlParserHomogeneousMatrix.h>
#include <visp3/core/vpXmlParserRectOriented.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".xml");
  static const vpCameraParameters::vpCameraParametersProjType kProj[] = {
    vpCameraParameters::perspectiveProjWithoutDistortion, vpCameraParameters::perspectiveProjWithDistortion,
    vpCameraParameters::ProjWithKannalaBrandtDistortion };
  for (auto proj : kProj) {
    FUZZ_TRY({
      vpXmlParserCamera p;
      vpCameraParameters cam;
      p.parse(cam, path, "Camera", proj, 0, 0, false);
    });
  }
  FUZZ_TRY({
    vpXmlParserCamera p;
    vpCameraParameters cam;
    p.parse(cam, path, "Camera", vpCameraParameters::perspectiveProjWithDistortion, 640, 480, false);
  });
  FUZZ_TRY({
    vpXmlParserHomogeneousMatrix p;
    vpHomogeneousMatrix M;
    p.parse(M, path, "eMc");
  });
  FUZZ_TRY({
    vpXmlParserRectOriented p;
    p.parse(path);
    (void)p.getRectangle();
  });
  unlink(path.c_str());
  return 0;
}
