// AprilTag/ArUco detection on fuzzed pixels: vpDetectorAprilTag::detect(), which runs the vendored
// apriltag pipeline (decimate/blur, quad threshold, decode, homography) — the code upstream fix
// 392f3121 (minimum image size check in apriltag_detector_detect) lives in.
// Layout: [0]=family selector, [1]=width, [2]=height, [3]=quad_decimate, [4]=quad_sigma,
//         [5]=flags (pose estimation), rest = 8-bit grayscale pixels (zero padded).
#include "fuzz_common.h"
#include <vector>
#include <visp3/core/vpCameraParameters.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/core/vpImage.h>
#include <visp3/detection/vpDetectorAprilTag.h>

// Families whose code tables are cheap to build. The big families (Circle49h12, Standard52h13)
// take seconds and >100MB to initialize per detector and are left out.
static const vpDetectorAprilTag::vpAprilTagFamily kFamilies[] = {
  vpDetectorAprilTag::TAG_36h11,        vpDetectorAprilTag::TAG_36h10,     vpDetectorAprilTag::TAG_25h9,
  vpDetectorAprilTag::TAG_16h5,         vpDetectorAprilTag::TAG_CIRCLE21h7, vpDetectorAprilTag::TAG_STANDARD41h12,
#if defined(VISP_HAVE_APRILTAG_ARUCO)
  vpDetectorAprilTag::TAG_ARUCO_4x4_50, vpDetectorAprilTag::TAG_ARUCO_5x5_50, vpDetectorAprilTag::TAG_ARUCO_6x6_50,
  vpDetectorAprilTag::TAG_ARUCO_MIP_36h12,
#endif
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 6)
    return -1;
  const size_t nfam = sizeof(kFamilies) / sizeof(kFamilies[0]);
  vpDetectorAprilTag::vpAprilTagFamily fam = kFamilies[data[0] % nfam];
  unsigned int w = 1 + data[1] % 160;
  unsigned int h = 1 + data[2] % 160;
  float decimate = 1.0f + (data[3] % 4);
  float sigma = (data[4] % 16) / 8.0f;
  bool pose = data[5] & 1;
  data += 6;
  size -= 6;

  vpImage<unsigned char> I(h, w, 0);
  for (unsigned int i = 0; i < w * h && i < size; ++i)
    I.bitmap[i] = data[i];

  FUZZ_TRY({
    vpDetectorAprilTag detector(fam);
    detector.setAprilTagNbThreads(1);
    detector.setAprilTagQuadDecimate(decimate);
    detector.setAprilTagQuadSigma(sigma);
    if (pose) {
      vpCameraParameters cam(600, 600, w / 2.0, h / 2.0);
      std::vector<vpHomogeneousMatrix> cMo;
      detector.detect(I, 0.05, cam, cMo);
    }
    else {
      detector.detect(I);
    }
    for (size_t i = 0; i < detector.getNbObjects(); ++i) {
      (void)detector.getPolygon(i);
      (void)detector.getMessage(i);
    }
  });
  return 0;
}
