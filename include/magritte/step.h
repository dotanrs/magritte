//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef MAGRITTE_STEP_H
#define MAGRITTE_STEP_H

#include <filesystem>
#include <string>
#include <vector>

#include "common/file_data.h"
#include "magritte/macro.h"
#include "magritte/inputs.h"

namespace fs = std::filesystem;

struct MagritteRunOptions {
    fs::path input;
    fs::path output;
    std::vector<StepSpec> steps;
    MacroMap macros;
    bool overwrite = false;
    bool debug = false;
};

struct StepCommand;

/// Runs the complete CLI workflow: validates paths, parses commands, reads the
/// input, applies valid steps, and writes the output JPEG.
///
/// Invalid step command strings are reported and skipped. If the output
/// exists and overwrite is disabled, this function prompts before replacing it.
void process_image(const MagritteRunOptions &options);

/// Processes a newly created in-memory canvas and writes it as a JPEG.
///
/// Step names are descriptive labels; each command is parsed exactly as
/// the value passed to the CLI's `-s` option.
void process_created_image(
    const fs::path &output,
    FileData data,
    const std::vector<StepSpec> &steps,
    bool overwrite = false,
    bool debug = false,
    const MacroMap &macros = {}
);

#endif //MAGRITTE_STEP_H
