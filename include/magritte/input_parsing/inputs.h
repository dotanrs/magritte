//
// Created by Dotan Reis on 31/07/2026.
//

#ifndef MAGRITTE_INPUTS_H
#define MAGRITTE_INPUTS_H

#include <string>
#include <filesystem>

struct StepSpec {
    std::string name;
    std::string command;
};

struct CanvasConfig {
    std::filesystem::path file_name;
    std::size_t width;
    std::size_t height;
};

struct PatternReference {
    std::filesystem::path path;
};

#endif //MAGRITTE_INPUTS_H
