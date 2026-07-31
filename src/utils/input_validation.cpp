//
// Created by Dotan Reis on 26/07/2026.
//

#include "../../include/magritte/utils/input_validation.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "magritte/utils/logging.h"

namespace fs = std::filesystem;

namespace {
    bool has_jpeg_markers(const fs::path &path) {
        std::ifstream image(path, std::ios::binary);
        if (!image) {
            throw std::runtime_error("could not open input image");
        }

        std::array<std::uint8_t, 2> start{};
        image.read(reinterpret_cast<char *>(start.data()), static_cast<std::streamsize>(start.size()));
        if (image.gcount() != static_cast<std::streamsize>(start.size())) {
            return false;
        }

        image.seekg(-2, std::ios::end);
        if (!image) {
            return false;
        }

        std::array<std::uint8_t, 2> end{};
        image.read(reinterpret_cast<char *>(end.data()), static_cast<std::streamsize>(end.size()));

        return start[0] == 0xFF && start[1] == 0xD8 &&
               end[0] == 0xFF && end[1] == 0xD9;
    }
}

std::tuple<fs::path, fs::path> validate_input(const MagritteRunOptions &options) {
    const fs::path input = fs::absolute(options.input).lexically_normal();
    const fs::path output = fs::absolute(options.output).lexically_normal();

    if (input == output) {
        throw std::runtime_error("input and output paths must be different");
    }

    log(LogLevel::info, "Validating JPEG: " + input.string());
    std::error_code error;
    if (!fs::exists(input, error) || error) {
        throw std::runtime_error("input image does not exist: " + input.string());
    }
    if (!fs::is_regular_file(input, error) || error) {
        throw std::runtime_error("input path is not a regular file: " + input.string());
    }
    if (!has_jpeg_markers(input)) {
        throw std::runtime_error("input is not a valid JPEG file: " + input.string());
    }

    return {input, output};
}
