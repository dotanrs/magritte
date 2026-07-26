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


void process_image(const Options& options) {
    auto [input, output] = validate_input(options);

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