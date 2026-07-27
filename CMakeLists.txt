cmake_minimum_required(VERSION 3.20)
project(pixlie VERSION 0.1.0 LANGUAGES CXX)

add_executable(pixlie main.cpp
        src/drawing.cpp
        src/drawing_config.cpp
        include/pixlie/drawing_config.h
        src/utils/logging.cpp
        include/pixlie/utils/logging.h
        src/input_validation.cpp
        include/pixlie/input_validation.h
        src/processor.cpp
        include/pixlie/processor.h
        src/utils/file.cpp
        include/pixlie/utils/file.h
        include/pixlie/common/file_data.h
        src/parser.cpp
        include/pixlie/parser.h
        src/processors/layout/rotate.cpp
        include/pixlie/processors/rotate.h
        src/processors/layout/mirror.cpp
        include/pixlie/processors/mirror.h
        src/processors/filters/blur.cpp
        include/pixlie/processors/blur.h
        src/processors/toolbox/fisheye.cpp
        include/pixlie/processors/fisheye.h
        src/processors/drawing/flow_lines.cpp
        include/pixlie/processors/flow_lines.h
        src/processors/filters/lighting.cpp
        include/pixlie/processors/lighting.h
        include/pixlie/processors/assignment_processor.h
        src/processors/formulas/rgb_formula.cpp
        include/pixlie/processors/rgb_formula.h
        src/processors/localized/local_rgb.cpp
        include/pixlie/processors/local_rgb.h
        src/processors/localized/local_warp.cpp
        include/pixlie/processors/local_warp.h
        src/processors/formulas/saturation_formula.cpp
        include/pixlie/processors/saturation_formula.h
        src/processors/formulas/warp_formula.cpp
        include/pixlie/processors/warp_formula.h
        src/processors/loops/loop_processor.cpp
        include/pixlie/processors/loop_processor.h
        include/pixlie/processors/loop_assignment_processor.h
        src/processors/loops/loop_warp.cpp
        include/pixlie/processors/loop_warp.h
        src/processors/loops/loop_rgb.cpp
        include/pixlie/processors/loop_rgb.h
        src/processors/formulas/utils/formula_parse.cpp
        include/pixlie/processors/utils/formula_parse.h
        src/processors/formulas/utils/formula_apply.cpp
        include/pixlie/processors/utils/formula_apply.h
        src/processors/formulas/utils/image_sample.cpp
        include/pixlie/processors/utils/image_sample.h
        include/pixlie/processors/utils/argument_parse.h
        include/pixlie/processors/image_processor.h
        src/utils/string.cpp
        include/pixlie/utils/string.h)

target_include_directories(pixlie PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(pixlie PRIVATE cxx_std_20)

if (APPLE)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(IMAGE_IO_FRAMEWORK ImageIO REQUIRED)
    target_link_libraries(pixlie PRIVATE
            "${CORE_FOUNDATION_FRAMEWORK}"
            "${CORE_GRAPHICS_FRAMEWORK}"
            "${IMAGE_IO_FRAMEWORK}")
else ()
    message(FATAL_ERROR "JPEG encoding and decoding currently require macOS ImageIO")
endif ()

if (MSVC)
    target_compile_options(pixlie PRIVATE /W4 /permissive-)
else ()
    target_compile_options(pixlie PRIVATE -Wall -Wextra -Wpedantic)
endif ()

install(TARGETS pixlie RUNTIME DESTINATION bin)

include(CTest)
if (BUILD_TESTING)
    add_executable(pixlie_processor_tests
            tests/processors_test.cpp
            tests/processors/blur_test.cpp
            tests/processors/fisheye_test.cpp
            tests/processors/flow_lines_test.cpp
            tests/processors/lighting_test.cpp
            tests/processors/local_rgb_test.cpp
            tests/processors/local_warp_test.cpp
            tests/processors/loop_rgb_test.cpp
            tests/processors/loop_warp_test.cpp
            tests/processors/mirror_test.cpp
            tests/processors/parser_test.cpp
            tests/drawing_config_test.cpp
            tests/processors/rgb_formula_test.cpp
            tests/processors/rotate_test.cpp
            tests/processors/saturation_formula_test.cpp
            tests/processors/warp_formula_test.cpp
            src/parser.cpp
            src/drawing_config.cpp
            src/processors/layout/rotate.cpp
            src/processors/layout/mirror.cpp
            src/processors/filters/blur.cpp
            src/processors/toolbox/fisheye.cpp
            src/processors/drawing/flow_lines.cpp
            src/processors/filters/lighting.cpp
            src/processors/formulas/rgb_formula.cpp
            src/processors/localized/local_rgb.cpp
            src/processors/localized/local_warp.cpp
            src/processors/formulas/saturation_formula.cpp
            src/processors/formulas/warp_formula.cpp
            src/processors/loops/loop_processor.cpp
            src/processors/loops/loop_warp.cpp
            src/processors/loops/loop_rgb.cpp
            src/processors/formulas/utils/formula_parse.cpp
            src/processors/formulas/utils/formula_apply.cpp
            src/processors/formulas/utils/image_sample.cpp
            src/utils/logging.cpp
            src/utils/string.cpp
            include/pixlie/utils/string.h)
    target_include_directories(pixlie_processor_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(pixlie_processor_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(pixlie_processor_tests PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(pixlie_processor_tests PRIVATE
                -Wall -Wextra -Wpedantic)
    endif ()

    add_test(NAME processors COMMAND pixlie_processor_tests)
endif ()
