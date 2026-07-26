//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_PROCESSOR_H
#define PIXLIE_PROCESSOR_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Pixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

static_assert(sizeof(Pixel) == 4);

struct FileData {
    std::size_t width;
    std::size_t height;
    std::vector<Pixel> pixels;
};

struct Options {
    fs::path input;
    fs::path output;
    std::vector<std::string> processor_commands;
};

struct ProcessorCommand;

FileData read_file(const fs::path& input);
FileData process_file(
    FileData data,
    const std::vector<ProcessorCommand>& commands
);
void process_image(const Options& options);

#endif //PIXLIE_PROCESSOR_H
