#include "magritte/inputs.h"
#include "magritte/pipeline.h"

#include <stdexcept>
#include <utility>
#include <vector>

#include "magritte/processor.h"
#include "magritte/common/file_data.h"

namespace fs = std::filesystem;

namespace {
    fs::path default_output_path(const fs::path &input) {
        return input.parent_path() /
               (input.stem().string() + "_copy" + input.extension().string());
    }
}

void process_pipeline(
    const std::vector<PipelineStep> &steps,
    bool overwrite,
    bool debug,
    const std::optional<fs::path> &source,
    const std::optional<fs::path> &output_override,
    const MacroMap &cli_macros
) {
    ResolvedPipeline resolved =
        resolve_pipeline_steps(steps, source.has_value(), cli_macros);

    if (source) {
        MagritteRunOptions options{
            .input = *source,
            .output = output_override.value_or(
                default_output_path(*source)
            ),
            .processors = std::move(resolved.processors),
            .macros = std::move(resolved.macros),
            .overwrite = overwrite,
            .debug = debug,
        };
        process_image(options);
        return;
    }

    if (!resolved.canvas) {
        throw std::logic_error(
            "a pipeline without a source must resolve to a canvas"
        );
    }
    const CanvasConfig &canvas_config = *resolved.canvas;
    FileData canvas{
        .width = canvas_config.width,
        .height = canvas_config.height,
        .pixels = std::vector<Pixel>(
            canvas_config.width * canvas_config.height,
            Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
        ),
    };
    process_created_image(
        output_override.value_or(canvas_config.file_name),
        std::move(canvas),
        resolved.processors,
        overwrite,
        debug,
        resolved.macros
    );
}
