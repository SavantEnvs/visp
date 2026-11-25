# Builds the ViSP fuzz harnesses against the sanitized ViSP build tree (VISP_DIR), twice each:
# <name> (libFuzzer) and <name>-standalone (run-once reproducer on $STANDALONE_FUZZ_MAIN).
cmake_minimum_required(VERSION 3.16)
project(visp_fuzz C CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(VISP REQUIRED COMPONENTS visp_core visp_io visp_detection visp_mbt visp_robot)
include_directories(${VISP_INCLUDE_DIRS})

set(HARNESSES
  fuzz_apriltag_detect fuzz_image_read_stb fuzz_image_read_portable fuzz_image_read_exr
  fuzz_mbt_cao_model fuzz_mbt_xml_config fuzz_npy_load fuzz_xml_parsers fuzz_array2d_load
  fuzz_config_file fuzz_wireframe_scene fuzz_mbt_json_config)

add_library(lsan_off OBJECT ${LSAN_OFF_SRC})
add_library(standalone_main OBJECT ${STANDALONE_FUZZ_MAIN})
separate_arguments(FUZZ_ENGINE_LIST UNIX_COMMAND "${LIB_FUZZING_ENGINE}")

foreach(h ${HARNESSES})
  add_executable(${h} ${h}.cpp $<TARGET_OBJECTS:lsan_off>)
  target_link_libraries(${h} ${VISP_LIBRARIES})
  target_link_options(${h} PRIVATE ${FUZZ_ENGINE_LIST})
  add_executable(${h}-standalone ${h}.cpp $<TARGET_OBJECTS:lsan_off> $<TARGET_OBJECTS:standalone_main>)
  target_link_libraries(${h}-standalone ${VISP_LIBRARIES})
  target_link_options(${h}-standalone PRIVATE -fsanitize=fuzzer-no-link)
endforeach()
