#ifndef MAGRITTE_FORMULA_H
#define MAGRITTE_FORMULA_H

#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "magritte/macro.h"
#include "magritte/inputs.h"



using PipelineStep = std::variant<ProcessorSpec, FormulaReference>;

struct FormulaConfig {
    std::optional<CanvasConfig> canvas;
    MacroMap macros;
    std::vector<PipelineStep> steps;
};

struct ResolvedPipeline {
    std::optional<CanvasConfig> canvas;
    MacroMap macros;
    std::vector<ProcessorSpec> processors;
};

/// Parses magritte's YAML formula schema from a stream.
///
/// This intentionally supports the small YAML subset used by formula files:
/// mappings, processor sequence items, sub-formula references, comments, and
/// plain, quoted, folded (`>`), or literal (`|`) scalars.
/// A canvas is optional at parse time because `--source` can supply the image.
/// @throws std::invalid_argument for malformed or incomplete input.
[[nodiscard]] FormulaConfig parse_formula_config(
    std::istream &input,
    std::string_view source_name = "<formula>"
);

/// Loads a formula YAML file. Relative canvas filenames and sub-formula
/// references are resolved beside the formula file.
[[nodiscard]] FormulaConfig load_formula_config(
    const std::filesystem::path &path
);

/// Recursively replaces formula references with their processors. When there
/// is no source, the first top-level step must be a formula that includes a
/// canvas, either directly or through a sub-formula.
/// CLI and file macros are collected before processing. Conflicting macros and
/// cyclic formula references are rejected.
[[nodiscard]] ResolvedPipeline resolve_pipeline_steps(
    const std::vector<PipelineStep> &steps,
    bool has_source,
    const MacroMap &cli_macros = {}
);

/// Expands and runs an ordered stream of processor and formula steps.
///
/// Without `source`, the first step must be a formula that includes a canvas.
/// Later formula canvases are ignored. `output_override`
/// replaces the canvas filename or the source image's default output.
void process_pipeline(
    const std::vector<PipelineStep> &steps,
    bool overwrite = false,
    bool debug = false,
    const std::optional<std::filesystem::path> &source = std::nullopt,
    const std::optional<std::filesystem::path> &output_override = std::nullopt,
    const MacroMap &cli_macros = {}
);

#endif // MAGRITTE_FORMULA_H
