//
// Created by Dotan Reis on 26/07/2026.
//

#include "pixlie/processor.h"
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include "pixlie/inputValidation.h"
#include "pixlie/utils/logging.h"


namespace fs = std::filesystem;

void copy_image(const Options& options) {
    const fs::path input = fs::absolute(options.input).lexically_normal();
    const fs::path output = fs::absolute(options.output).lexically_normal();

    if (input == output) {
        throw std::runtime_error("input and output paths must be different");
    }

    log(LogLevel::info, "Reading JPEG: " + input.string());
    validate_input(input);

    if (!output.parent_path().empty()) {
        std::error_code error;
        fs::create_directories(output.parent_path(), error);
        if (error) {
            throw std::runtime_error("could not create output directory: " + error.message());
        }
    }

    log(LogLevel::info, "Writing copy: " + output.string());
    std::error_code error;
    fs::copy_file(input, output, fs::copy_options::overwrite_existing, error);
    if (error) {
        throw std::runtime_error("could not copy image: " + error.message());
    }

    const auto bytes = fs::file_size(output, error);
    if (error) {
        throw std::runtime_error("could not read output size: " + error.message());
    }
    log(LogLevel::info, "Copy complete (" + std::to_string(bytes) + " bytes)");
}