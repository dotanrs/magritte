//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_PROCESSOR_H
#define PIXLIE_PROCESSOR_H

#include <filesystem>

namespace fs = std::filesystem;

struct Options {
    fs::path input;
    fs::path output;
};

void copy_image(const Options& options);

#endif //PIXLIE_PROCESSOR_H
