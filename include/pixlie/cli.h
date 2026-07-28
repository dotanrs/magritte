#ifndef PIXLIE_CLI_H
#define PIXLIE_CLI_H

#include <filesystem>
#include <optional>
#include <vector>

#include "pixlie/formula.h"

struct CommandLineArguments {
    std::optional<std::filesystem::path> source;
    std::optional<std::filesystem::path> output;
    std::vector<PipelineStep> steps;
    bool overwrite = false;
    bool debug = false;
};

/// Parses the flag-only CLI. `-p` and `-f` may be repeated and are retained in
/// their original order. Without `--source`, the first processing argument
/// must be `-f`; its canvas is validated when the formula is loaded.
/// @throws std::invalid_argument when arguments are missing or unsupported.
[[nodiscard]] CommandLineArguments parse_command_line(
    int argc,
    char *argv[]
);

#endif // PIXLIE_CLI_H
