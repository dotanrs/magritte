//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_PROCESSOR_H
#define PIXLIE_PROCESSOR_H

#include <filesystem>
#include <string>
#include <vector>

#include "common/file_data.h"

namespace fs = std::filesystem;

struct Options {
    fs::path input;
    fs::path output;
    std::vector<std::string> processor_commands;
    bool overwrite = false;
    bool debug = false;
};

struct ProcessorCommand;

/// Applies validated processor commands in order, optionally adding each
/// processor's visual debug hints, and checks the image invariant after every
/// transformation.
FileData process_file(
    FileData data,
    const std::vector<ProcessorCommand> &commands,
    bool debug = false
);

/// Runs the complete CLI workflow: validates paths, parses commands, reads the
/// input, applies valid processors, and writes the output JPEG.
///
/// Invalid processor command strings are reported and skipped. If the output
/// exists and overwrite is disabled, this function prompts before replacing it.
void process_image(const Options &options);

#endif //PIXLIE_PROCESSOR_H
