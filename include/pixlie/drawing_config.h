#ifndef PIXLIE_DRAWING_CONFIG_H
#define PIXLIE_DRAWING_CONFIG_H

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "pixlie/processor.h"

struct CanvasConfig {
    std::filesystem::path file_name;
    std::size_t width;
    std::size_t height;
};

struct DrawingConfig {
    std::optional<CanvasConfig> canvas;
    std::optional<std::filesystem::path> source_image;
    std::vector<ProcessorSpec> processors;
};

/// Parses pixlie's YAML drawing schema from a stream.
///
/// This intentionally supports the small YAML subset used by drawing files:
/// mappings, processor sequence items, comments, and quoted or plain scalars.
/// Exactly one of `canvas` and `source_image` must be provided.
/// @throws std::invalid_argument for malformed or incomplete input.
[[nodiscard]] DrawingConfig parse_drawing_config(
    std::istream &input,
    std::string_view source_name = "<drawing>"
);

/// Loads a drawing YAML file. Relative canvas filenames are resolved beside
/// the drawing file, rather than against the process working directory.
[[nodiscard]] DrawingConfig load_drawing_config(
    const std::filesystem::path &path
);

/// Runs the processors described by `path`. `source_override` replaces either
/// YAML input mode, and `output_override` replaces the configured/default
/// destination.
void process_drawing(
    const std::filesystem::path &path,
    bool overwrite = false,
    bool debug = false,
    const std::optional<std::filesystem::path> &source_override = std::nullopt,
    const std::optional<std::filesystem::path> &output_override = std::nullopt
);

#endif // PIXLIE_DRAWING_CONFIG_H
