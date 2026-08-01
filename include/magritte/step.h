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
    FileData file_data;
    std::vector<StepSpec> steps;
    MacroMap macros;
    bool overwrite = false;
    bool debug = false;
};

struct StepCommand;

/// Runs the list of steps on the image.
/// Invalid step command strings are reported and skipped.
FileData process_image(const MagritteRunOptions &options);


#endif //MAGRITTE_STEP_H
