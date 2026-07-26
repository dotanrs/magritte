//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_PROCESSOR_H
#define PIXLIE_PROCESSOR_H

#include <filesystem>
#include <string>
#include <vector>

#include "pixlie/file_utils/file_data.h"

namespace fs = std::filesystem;

struct Options {
    fs::path input;
    fs::path output;
    std::vector<std::string> processor_commands;
};

struct ProcessorCommand;

FileData process_file(
    FileData data,
    const std::vector<ProcessorCommand> &commands
);

void process_image(const Options &options);

#endif //PIXLIE_PROCESSOR_H
