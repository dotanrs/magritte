//
// Created by Dotan Reis on 26/07/2026.
//

#include "pixlie/processor.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include "pixlie/inputValidation.h"
#include "pixlie/utils/logging.h"


namespace fs = std::filesystem;

namespace {
    void save_file(const fs::path& output, const FileData& data) {
        if (!output.parent_path().empty()) {
            std::error_code error;
            fs::create_directories(output.parent_path(), error);
            if (error) {
                throw std::runtime_error("could not create output directory: " + error.message());
            }
        }

        log(LogLevel::info, "Saving file: " + output.string());
        std::ofstream file(output, std::ios::binary | std::ios::trunc);
        if (!file) {
            throw std::runtime_error("could not open output file: " + output.string());
        }

        if (!data.empty()) {
            file.write(data.data(), static_cast<std::streamsize>(data.size()));
        }
        if (!file) {
            throw std::runtime_error("could not write output file: " + output.string());
        }
    }
}

FileData read_file(const fs::path& input) {
    log(LogLevel::info, "Reading file: " + input.string());

    std::ifstream file(input, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("could not open input file: " + input.string());
    }

    const std::streampos end = file.tellg();
    if (end < 0) {
        throw std::runtime_error("could not determine input file size: " + input.string());
    }

    FileData data(static_cast<std::size_t>(end));
    file.seekg(0, std::ios::beg);
    if (!data.empty()) {
        file.read(data.data(), static_cast<std::streamsize>(data.size()));
    }
    if (!file) {
        throw std::runtime_error("could not read input file: " + input.string());
    }

    log(LogLevel::info, "Read " + std::to_string(data.size()) + " bytes");
    return data;
}

FileData process_file(FileData data) {
    log(LogLevel::info, "Processing " + std::to_string(data.size()) + " bytes");

    // Processing instructions will transform data here.
    return data;
}

void process_image(const Options& options) {
    auto [input, output] = validate_input(options);
    const FileData data = process_file(read_file(input));
    save_file(output, data);
    log(LogLevel::info, "Processing complete (" + std::to_string(data.size()) + " bytes)");
}
