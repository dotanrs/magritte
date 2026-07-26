cmake_minimum_required(VERSION 3.20)
project(pixlie VERSION 0.1.0 LANGUAGES CXX)

add_executable(pixlie main.cpp
        src/utils/logging.cpp
        include/pixlie/utils/logging.h
        src/inputValidation.cpp
        include/pixlie/inputValidation.h
        src/processor.cpp
        include/pixlie/processor.h)

target_include_directories(pixlie PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(pixlie PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(pixlie PRIVATE /W4 /permissive-)
else()
    target_compile_options(pixlie PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS pixlie RUNTIME DESTINATION bin)

