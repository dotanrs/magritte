#ifndef MAGRITTE_PIPELINE_H
#define MAGRITTE_PIPELINE_H

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "magritte/macro.h"
#include "magritte/inputs.h"



using PipelineStage = std::variant<StepSpec, PatternReference>;

struct PatternConfig {
    std::optional<CanvasConfig> canvas;
    MacroMap macros;
    std::vector<PipelineStage> steps;
};

struct ResolvedPipeline {
    std::optional<CanvasConfig> canvas;
    MacroMap macros;
    std::vector<StepSpec> steps;
};

/// Parses magritte's YAML pattern schema from a stream.
///
/// This intentionally supports the small YAML subset used by pattern files:
/// mappings, step sequence items, nested pattern references, comments, and
/// plain, quoted, folded (`>`), or literal (`|`) scalars.
/// A canvas is optional at parse time because `--source` can supply the image.
/// @throws std::invalid_argument for malformed or incomplete input.
[[nodiscard]] PatternConfig parse_pattern_config(
    std::istream &input,
    std::string_view source_name = "<pattern>"
);

/// Loads a pattern YAML file. Relative canvas filenames and nested pattern
/// references are resolved beside the pattern file.
[[nodiscard]] PatternConfig load_pattern_config(
    const std::filesystem::path &path
);

/// Recursively replaces pattern references with their steps. When there
/// is no source, the first top-level step must be a pattern that includes a
/// canvas, either directly or through a nested pattern.
/// CLI and file macros are collected before processing. Conflicting macros and
/// cyclic pattern references are rejected.
[[nodiscard]] ResolvedPipeline resolve_pipeline_steps(
    const std::vector<PipelineStage> &steps,
    bool has_source,
    const MacroMap &cli_macros = {}
);

/// Expands and runs an ordered stream of inline steps and patterns.
///
/// Without `source`, the first step must be a pattern that includes a canvas.
/// Later pattern canvases are ignored. `output_override`
/// replaces the canvas filename or the source image's default output.
void process_pipeline(
    const std::vector<PipelineStage> &steps,
    bool overwrite = false,
    bool debug = false,
    const std::optional<std::filesystem::path> &source = std::nullopt,
    const std::optional<std::filesystem::path> &output_override = std::nullopt,
    const MacroMap &cli_macros = {}
);

#endif // MAGRITTE_PIPELINE_H
