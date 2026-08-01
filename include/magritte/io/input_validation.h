//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef MAGRITTE_INPUTVALIDATION_H
#define MAGRITTE_INPUTVALIDATION_H

#include <tuple>
#include "magritte/step.h"

/// Resolves the input and output to normalized absolute paths and verifies that
/// the input is a distinct regular file with JPEG boundary markers.
/// @throws std::runtime_error if a path or input-file check fails.
std::tuple<fs::path, fs::path> validate_io_paths(const fs::path &input_path, const fs::path &output_path);

#endif //MAGRITTE_INPUTVALIDATION_H
