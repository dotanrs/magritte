//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_PROCESSOR_H
#define PIXLIE_PROCESSOR_H

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

using FileData = std::vector<char>;

struct Options {
    fs::path input;
    fs::path output;
};

FileData read_file(const fs::path& input);
FileData process_file(FileData data);
void process_image(const Options& options);

#endif //PIXLIE_PROCESSOR_H
