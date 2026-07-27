#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "pixlie/common/file_data.h"

inline int failures = 0;

inline void expect(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

inline std::vector<std::uint8_t> red_values(const FileData &data) {
    std::vector<std::uint8_t> values;
    values.reserve(data.pixels.size());
    for (const Pixel &pixel: data.pixels) {
        values.push_back(pixel.red);
    }
    return values;
}

inline FileData blank_image(std::size_t width, std::size_t height) {
    return FileData{
        .width = width,
        .height = height,
        .pixels = std::vector<Pixel>(
            width * height,
            Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
        ),
    };
}
