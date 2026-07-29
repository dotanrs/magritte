//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef MAGRITTE_PROCESSOR_H
#define MAGRITTE_PROCESSOR_H

#include <filesystem>
#include <string>
#include <vector>

#include "common/file_data.h"
#include "magritte/macro.h"

namespace fs = std::filesystem;

struct ProcessorSpec {
    std::string name;
    std::string command;
};

struct Options {
    fs::path input;
    fs::path output;
    std::vector<ProcessorSpec> processors;
    MacroMap macros;
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
    bool debug = false,
    const MacroMap *macros = nullptr
);

/// Runs the complete CLI workflow: validates paths, parses commands, reads the
/// input, applies valid processors, and writes the output JPEG.
///
/// Invalid processor command strings are reported and skipped. If the output
/// exists and overwrite is disabled, this function prompts before replacing it.
void process_image(const Options &options);

/// Processes a newly created in-memory canvas and writes it as a JPEG.
///
/// Processor names are descriptive labels; each command is parsed exactly as
/// the value passed to the CLI's `-p` option.
void process_created_image(
    const fs::path &output,
    FileData data,
    const std::vector<ProcessorSpec> &processors,
    bool overwrite = false,
    bool debug = false,
    const MacroMap &macros = {}
);

#endif //MAGRITTE_PROCESSOR_H
