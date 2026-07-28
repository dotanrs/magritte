cmake_minimum_required(VERSION 3.20)
project(magritte VERSION 0.1.0 LANGUAGES CXX)

add_executable(magritte main.cpp
        src/cli.cpp
        include/magritte/cli.h
        src/formula.cpp
        src/formula_config.cpp
        include/magritte/formula.h
        src/utils/logging.cpp
        include/magritte/utils/logging.h
        src/input_validation.cpp
        include/magritte/input_validation.h
        src/processor.cpp
        include/magritte/processor.h
        src/utils/file.cpp
        include/magritte/utils/file.h
        include/magritte/common/file_data.h
        src/parser.cpp
        include/magritte/parser.h
        src/processors/layout/rotate.cpp
        include/magritte/processors/rotate.h
        src/processors/layout/mirror.cpp
        include/magritte/processors/mirror.h
        src/processors/filters/blur.cpp
        include/magritte/processors/blur.h
        src/processors/toolbox/black_and_white.cpp
        include/magritte/processors/black_and_white.h
        src/processors/toolbox/contrast.cpp
        include/magritte/processors/contrast.h
        src/processors/toolbox/fisheye.cpp
        include/magritte/processors/fisheye.h
        src/processors/toolbox/twist.cpp
        include/magritte/processors/twist.h
        src/processors/toolbox/spin.cpp
        include/magritte/processors/spin.h
        src/processors/drawing/flow_lines.cpp
        include/magritte/processors/flow_lines.h
        src/processors/filters/lighting.cpp
        include/magritte/processors/lighting.h
        include/magritte/processors/assignment_processor.h
        src/processors/formulas/rgb_formula.cpp
        include/magritte/processors/rgb_formula.h
        src/processors/localized/local_rgb.cpp
        include/magritte/processors/local_rgb.h
        src/processors/localized/local_warp.cpp
        include/magritte/processors/local_warp.h
        src/processors/formulas/saturation_formula.cpp
        include/magritte/processors/saturation_formula.h
        src/processors/formulas/warp_formula.cpp
        include/magritte/processors/warp_formula.h
        src/processors/loops/loop_processor.cpp
        include/magritte/processors/loop_processor.h
        include/magritte/processors/loop_assignment_processor.h
        src/processors/loops/loop_warp.cpp
        include/magritte/processors/loop_warp.h
        src/processors/loops/loop_rgb.cpp
        include/magritte/processors/loop_rgb.h
        src/processors/formulas/utils/formula_parse.cpp
        include/magritte/processors/utils/formula_parse.h
        src/processors/formulas/utils/formula_apply.cpp
        include/magritte/processors/utils/formula_apply.h
        src/processors/formulas/utils/image_sample.cpp
        include/magritte/processors/utils/image_sample.h
        include/magritte/processors/utils/argument_parse.h
        include/magritte/processors/image_processor.h
        src/utils/string.cpp
        include/magritte/utils/string.h)

target_include_directories(magritte PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(magritte PRIVATE cxx_std_20)

if (APPLE)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(IMAGE_IO_FRAMEWORK ImageIO REQUIRED)
    target_link_libraries(magritte PRIVATE
            "${CORE_FOUNDATION_FRAMEWORK}"
            "${CORE_GRAPHICS_FRAMEWORK}"
            "${IMAGE_IO_FRAMEWORK}")
else ()
    message(FATAL_ERROR "JPEG encoding and decoding currently require macOS ImageIO")
endif ()

if (MSVC)
    target_compile_options(magritte PRIVATE /W4 /permissive-)
else ()
    target_compile_options(magritte PRIVATE -Wall -Wextra -Wpedantic)
endif ()

install(TARGETS magritte RUNTIME DESTINATION bin)

include(CTest)
if (BUILD_TESTING)
    add_executable(magritte_processor_tests
            tests/processors_test.cpp
            tests/cli_test.cpp
            tests/processors/blur_test.cpp
            tests/processors/black_and_white_test.cpp
            tests/processors/contrast_test.cpp
            tests/processors/fisheye_test.cpp
            tests/processors/flow_lines_test.cpp
            tests/processors/lighting_test.cpp
            tests/processors/twist_test.cpp
            tests/processors/spin_test.cpp
            tests/processors/local_rgb_test.cpp
            tests/processors/local_warp_test.cpp
            tests/processors/loop_rgb_test.cpp
            tests/processors/loop_warp_test.cpp
            tests/processors/mirror_test.cpp
            tests/processors/parser_test.cpp
            tests/formula_config_test.cpp
            tests/processors/rgb_formula_test.cpp
            tests/processors/rotate_test.cpp
            tests/processors/saturation_formula_test.cpp
            tests/processors/warp_formula_test.cpp
            src/cli.cpp
            src/parser.cpp
            src/formula_config.cpp
            src/processors/layout/rotate.cpp
            src/processors/layout/mirror.cpp
            src/processors/filters/blur.cpp
            src/processors/toolbox/black_and_white.cpp
            src/processors/toolbox/contrast.cpp
            src/processors/toolbox/fisheye.cpp
            src/processors/toolbox/twist.cpp
            src/processors/toolbox/spin.cpp
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
            include/magritte/utils/string.h)
    target_include_directories(magritte_processor_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(magritte_processor_tests PRIVATE cxx_std_20)

    if (MSVC)
        target_compile_options(magritte_processor_tests PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(magritte_processor_tests PRIVATE
                -Wall -Wextra -Wpedantic)
    endif ()

    add_test(NAME processors COMMAND magritte_processor_tests)
endif ()
