//
// Created by Dotan Reis on 01/08/2026.
//

#ifndef MAGRITTE_OUTPUT_FILE_VALIDATION_H
#define MAGRITTE_OUTPUT_FILE_VALIDATION_H

#include <filesystem>

namespace fs = std::filesystem;

bool confirm_overwrite(const fs::path &output);

bool should_write_output(const fs::path &output, bool overwrite);

#endif //MAGRITTE_OUTPUT_FILE_VALIDATION_H
