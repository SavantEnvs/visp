#!/usr/bin/env bash
#
# mayhem/build.sh — builds ViSP (sanitized) + the fuzz harnesses, and a normal-flags build of a set of
# upstream unit tests for mayhem/test.sh. Re-runnable and offline: every dependency used is vendored
# in 3rdparty/ (apriltag, pugixml, stb_image, tinyexr, miniz, simdlib, clapack); optional system
# libraries (OpenCV, X11, libjpeg/libpng/zlib, Eigen, nlohmann_json, ...) are switched off so the
# embedded code paths are the ones fuzzed.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# Build trees live under build/ (gitignored upstream), so `git clean -ffdX` removes them.
VISP_OPTS=(
  -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DEMOS=OFF -DBUILD_TUTORIALS=OFF -DBUILD_APPS=OFF
  -DBUILD_JAVA=OFF -DBUILD_PYTHON_BINDINGS=OFF -DBUILD_MODULE_visp_ar=OFF
  -DBUILD_MODULE_visp_sensor=OFF -DBUILD_MODULE_visp_imgproc=OFF -DBUILD_MODULE_visp_tt=OFF
  -DBUILD_MODULE_visp_tt_mi=OFF -DBUILD_MODULE_visp_rbt=OFF
  -DUSE_OPENMP=OFF -DUSE_X11=OFF -DUSE_GTK2=OFF -DUSE_OPENCV=OFF -DUSE_PCL=OFF -DUSE_EIGEN3=OFF
  -DUSE_ZLIB=OFF -DUSE_JPEG=OFF -DUSE_PNG=OFF -DUSE_XML2=OFF -DUSE_NLOHMANN_JSON=OFF -DUSE_COIN3D=OFF
  -DUSE_PANDA3D=OFF -DUSE_OGRE=OFF -DUSE_QT=OFF -DUSE_ZBAR=OFF -DUSE_DMTX=OFF -DUSE_V4L2=OFF -DUSE_DC1394=OFF
  -DUSE_LIBUSB_1=OFF -DUSE_REALSENSE2=OFF -DUSE_FFMPEG=OFF -DUSE_PYBIND11=OFF -DENABLE_MOMENTS_COMBINE_MATRICES=OFF
)

# Fuzz compile flags. -fsanitize=fuzzer-no-link puts SanCov on the ViSP code itself (not only the harness
# TU). UBSan's nonnull-attribute check is relaxed: vpImage::operator= (vpImage_operators.h) memcpy()s
# from a null bitmap with size 0 whenever an empty image is copied, which the tracker does on every
# construction, so the check would fire on every input. ASan and the rest of UBSan stay halting.
FUZZ_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link -fno-sanitize=nonnull-attribute $DEBUG_FLAGS -O1"

# 1) Sanitized ViSP (the fuzzed code is instrumented) + DWARF 3.
#    nlohmann_json (apt, header-only) is enabled here only, for the JSON tracker-settings target.
cmake -S "$SRC" -B build/fuzz -DCMAKE_BUILD_TYPE=Debug "${VISP_OPTS[@]}" -DUSE_NLOHMANN_JSON=ON -DBUILD_TESTS=OFF \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_C_FLAGS_DEBUG="" -DCMAKE_CXX_FLAGS_DEBUG=""
cmake --build build/fuzz -j"$MAYHEM_JOBS" --target visp_core visp_io visp_detection visp_mbt visp_robot

# 2) Harnesses (libFuzzer + standalone), linked against the sanitized static libs; mayhem/lsan_off.cc
#    (LeakSanitizer off at build time) is compiled with the same flags and linked into every binary.
cmake -S "$SRC/mayhem/harnesses" -B build/harnesses -DCMAKE_BUILD_TYPE=Debug -DVISP_DIR="$SRC/build/fuzz" \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_C_FLAGS_DEBUG="" -DCMAKE_CXX_FLAGS_DEBUG="" \
  -DLSAN_OFF_SRC="$SRC/mayhem/lsan_off.cc" -DLIB_FUZZING_ENGINE="$LIB_FUZZING_ENGINE" -DSTANDALONE_FUZZ_MAIN="$STANDALONE_FUZZ_MAIN"
cmake --build build/harnesses -j"$MAYHEM_JOBS"

# 3) Upstream unit tests with NORMAL flags (independent of the sanitized build) for mayhem/test.sh.
cmake -S "$SRC" -B build/test -DCMAKE_BUILD_TYPE=Release "${VISP_OPTS[@]}" -DBUILD_TESTS=ON \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build/test -j"$MAYHEM_JOBS" --target $(cat "$SRC/mayhem/tests.txt")
