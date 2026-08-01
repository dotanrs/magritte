#ifndef MAGRITTE_CLI_H
#define MAGRITTE_CLI_H

#include <filesystem>
#include <optional>
#include <vector>

#include "magritte/pipeline.h"
#include "magritte/macro.h"

struct CommandLineArguments {
    std::optional<std::filesystem::path> source;
    std::optional<std::filesystem::path> output;
    std::vector<PipelineStage> steps;
    MacroMap macros;
    bool overwrite = false;
    bool debug = false;
};

/// Parses the flag-only CLI. `-s`, `-p`, and `--macro` may be repeated; the
/// processing steps retain their original order. Without `--source`, the first
/// processing argument must be `-p`; its canvas is validated when loaded.
/// @throws std::invalid_argument when arguments are missing or unsupported.
[[nodiscard]] CommandLineArguments parse_command_line(
    int argc,
    char *argv[]
);

#endif // MAGRITTE_CLI_H
