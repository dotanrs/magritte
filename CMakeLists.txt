cmake_minimum_required(VERSION 3.20)
project(pixlie VERSION 0.1.0 LANGUAGES CXX)

add_executable(pixlie main.cpp
        src/utils/logging.cpp
        include/pixlie/utils/logging.h
        src/inputValidation.cpp
        include/pixlie/inputValidation.h
        src/processor.cpp
        include/pixlie/processor.h
        src/file_utils/file_utils.cpp
        include/pixlie/file_utils/file_utils.h
        include/pixlie/file_utils/file_data.h
        src/parser.cpp
        include/pixlie/parser.h
        src/processors/rotate.cpp
        include/pixlie/processors/rotate.h
        src/processors/mirror.cpp
        include/pixlie/processors/mirror.h
        src/processors/blur.cpp
        include/pixlie/processors/blur.h
        src/processors/channel_formula.cpp
        include/pixlie/processors/assignment_processor.h
        include/pixlie/processors/red_formula.h
        include/pixlie/processors/green_formula.h
        include/pixlie/processors/blue_formula.h
        src/processors/rgb_formula.cpp
        include/pixlie/processors/rgb_formula.h
        src/processors/saturation_formula.cpp
        include/pixlie/processors/saturation_formula.h
        src/processors/color_swap.cpp
        include/pixlie/processors/color_swap.h
        src/processors/utils/formula_parse.cpp
        include/pixlie/processors/utils/formula_parse.h
        src/processors/utils/formula_apply.cpp
        include/pixlie/processors/utils/formula_apply.h
        include/pixlie/processors/utils/argument_parse.h
        include/pixlie/processors/image_processor.h)

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
            src/parser.cpp
            src/processors/rotate.cpp
            src/processors/mirror.cpp
            src/processors/blur.cpp
            src/processors/channel_formula.cpp
            src/processors/rgb_formula.cpp
            src/processors/saturation_formula.cpp
            src/processors/color_swap.cpp
            src/processors/utils/formula_parse.cpp
            src/processors/utils/formula_apply.cpp
            src/utils/logging.cpp)
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
