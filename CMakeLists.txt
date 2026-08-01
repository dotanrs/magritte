cmake_minimum_required(VERSION 3.20)
project(magritte VERSION 0.1.0 LANGUAGES CXX)

option(MAGRITTE_ENABLE_CLANG_TIDY
        "Run clang-tidy while compiling magritte targets" OFF)

if (MAGRITTE_ENABLE_CLANG_TIDY)
    find_program(MAGRITTE_CLANG_TIDY_EXECUTABLE
            NAMES clang-tidy
            REQUIRED)
    set(MAGRITTE_CLANG_TIDY_COMMAND
            "${MAGRITTE_CLANG_TIDY_EXECUTABLE}"
            "--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")

    if (APPLE)
        execute_process(
                COMMAND xcrun --show-sdk-path
                OUTPUT_VARIABLE MAGRITTE_MACOS_SDK_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                COMMAND_ERROR_IS_FATAL ANY)
        list(APPEND MAGRITTE_CLANG_TIDY_COMMAND
                "--extra-arg-before=-isysroot"
                "--extra-arg-before=${MAGRITTE_MACOS_SDK_PATH}")
    endif ()
endif ()

function(magritte_enable_clang_tidy target)
    if (MAGRITTE_ENABLE_CLANG_TIDY)
        set_property(TARGET "${target}" PROPERTY CXX_CLANG_TIDY
                "${MAGRITTE_CLANG_TIDY_COMMAND}")
    endif ()
endfunction ()

add_executable(magritte main.cpp
        src/cli.cpp
        include/magritte/cli.h
        src/pipeline.cpp
        src/input_parsing/pattern_parser.cpp
        include/magritte/pipeline.h
        src/input_parsing/macro_parser.cpp
        include/magritte/input_parsing/macro_parser.h
        src/utils/logging.cpp
        include/magritte/utils/logging.h
        src/io/input_validation.cpp
        include/magritte/io/input_validation.h
        src/step_runner.cpp
        include/magritte/step.h
        src/io/file.cpp
        include/magritte/io/file.h
        include/magritte/common/file_data.h
        src/input_parsing/step_command_parser.cpp
        include/magritte/input_parsing/step_command_parser.h
        src/steps/layout/rotate.cpp
        include/magritte/steps/rotate.h
        src/steps/layout/mirror.cpp
        include/magritte/steps/mirror.h
        src/steps/filters/blur.cpp
        include/magritte/steps/blur.h
        src/steps/toolbox/black_and_white.cpp
        include/magritte/steps/black_and_white.h
        src/steps/toolbox/contrast.cpp
        include/magritte/steps/contrast.h
        src/steps/toolbox/fisheye.cpp
        include/magritte/steps/fisheye.h
        src/steps/toolbox/twist.cpp
        include/magritte/steps/twist.h
        src/steps/toolbox/spin.cpp
        include/magritte/steps/spin.h
        src/steps/drawing/flow_lines.cpp
        include/magritte/steps/flow_lines.h
        src/steps/filters/lighting.cpp
        include/magritte/steps/lighting.h
        include/magritte/steps/assignment_step.h
        src/steps/formulas/rgb_formula.cpp
        include/magritte/steps/rgb_formula.h
        src/steps/localized/local_rgb.cpp
        include/magritte/steps/local_rgb.h
        src/steps/localized/local_warp.cpp
        include/magritte/steps/local_warp.h
        src/steps/formulas/saturation_formula.cpp
        include/magritte/steps/saturation_formula.h
        src/steps/formulas/warp_formula.cpp
        include/magritte/steps/warp_formula.h
        src/steps/loops/loop_step.cpp
        include/magritte/steps/loop_step.h
        include/magritte/steps/loop_assignment_step.h
        src/steps/loops/loop_warp.cpp
        include/magritte/steps/loop_warp.h
        src/steps/loops/loop_rgb.cpp
        include/magritte/steps/loop_rgb.h
        src/steps/formulas/utils/formula_parse.cpp
        include/magritte/steps/utils/formula_parse.h
        src/steps/formulas/utils/formula_apply.cpp
        include/magritte/steps/utils/formula_apply.h
        src/steps/formulas/utils/image_sample.cpp
        include/magritte/steps/utils/image_sample.h
        include/magritte/steps/utils/argument_parse.h
        include/magritte/steps/image_step.h
        src/utils/string.cpp
        include/magritte/utils/string.h
        include/magritte/input_parsing/inputs.h
        src/pipeline.cpp
        src/io/output_file_validation.cpp
        include/magritte/io/output_file_validation.h)

target_include_directories(magritte PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(magritte PRIVATE cxx_std_20)
magritte_enable_clang_tidy(magritte)

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
    add_executable(magritte_step_tests
            tests/main.cpp
            tests/cli_test.cpp
            tests/steps/blur_test.cpp
            tests/steps/black_and_white_test.cpp
            tests/steps/contrast_test.cpp
            tests/steps/fisheye_test.cpp
            tests/steps/flow_lines_test.cpp
            tests/steps/lighting_test.cpp
            tests/steps/twist_test.cpp
            tests/steps/spin_test.cpp
            tests/steps/local_rgb_test.cpp
            tests/steps/local_warp_test.cpp
            tests/steps/loop_rgb_test.cpp
            tests/steps/loop_warp_test.cpp
            tests/steps/mirror_test.cpp
            tests/steps/parser_test.cpp
            tests/pattern_config_test.cpp
            tests/steps/rgb_formula_test.cpp
            tests/steps/rotate_test.cpp
            tests/steps/saturation_formula_test.cpp
            tests/steps/warp_formula_test.cpp
            src/cli.cpp
            src/input_parsing/step_command_parser.cpp
            src/input_parsing/pattern_parser.cpp
            src/input_parsing/macro_parser.cpp
            src/steps/layout/rotate.cpp
            src/steps/layout/mirror.cpp
            src/steps/filters/blur.cpp
            src/steps/toolbox/black_and_white.cpp
            src/steps/toolbox/contrast.cpp
            src/steps/toolbox/fisheye.cpp
            src/steps/toolbox/twist.cpp
            src/steps/toolbox/spin.cpp
            src/steps/drawing/flow_lines.cpp
            src/steps/filters/lighting.cpp
            src/steps/formulas/rgb_formula.cpp
            src/steps/localized/local_rgb.cpp
            src/steps/localized/local_warp.cpp
            src/steps/formulas/saturation_formula.cpp
            src/steps/formulas/warp_formula.cpp
            src/steps/loops/loop_step.cpp
            src/steps/loops/loop_warp.cpp
            src/steps/loops/loop_rgb.cpp
            src/steps/formulas/utils/formula_parse.cpp
            src/steps/formulas/utils/formula_apply.cpp
            src/steps/formulas/utils/image_sample.cpp
            src/utils/logging.cpp
            src/utils/string.cpp
            include/magritte/utils/string.h
            src/io/output_file_validation.cpp
            include/magritte/io/output_file_validation.h)
    target_include_directories(magritte_step_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(magritte_step_tests PRIVATE cxx_std_20)
    magritte_enable_clang_tidy(magritte_step_tests)

    if (MSVC)
        target_compile_options(magritte_step_tests PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(magritte_step_tests PRIVATE
                -Wall -Wextra -Wpedantic)
    endif ()

    add_test(NAME steps COMMAND magritte_step_tests)
endif ()
