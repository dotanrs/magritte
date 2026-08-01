#include "magritte/inputs.h"
#include "magritte/pipeline.h"

#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>

#include "magritte/step.h"
#include "magritte/common/file_data.h"
#include "magritte/io/file.h"
#include "magritte/io/input_validation.h"
#include "magritte/io/output_file_validation.h"
#include "magritte/utils/logging.h"

namespace fs = std::filesystem;

namespace {
    fs::path default_output_path(const fs::path &input) {
        return input.parent_path() /
               (input.stem().string() + "_copy" + input.extension().string());
    }

    FileData build_canvas(const ResolvedPipeline &resolved_pipeline) {
        // Building from canvas
        if (!resolved_pipeline.canvas) {
            throw std::logic_error(
                "a pipeline without a source must resolve to a canvas"
            );
        }
        const CanvasConfig &canvas_config = *resolved_pipeline.canvas;
        return {
            .width = canvas_config.width,
            .height = canvas_config.height,
            .pixels = std::vector<Pixel>(
                canvas_config.width * canvas_config.height,
                Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
            ),
        };
    }

    std::pair<FileData, fs::path> resolve_input_data_and_output(const std::optional<fs::path> &source, const std::optional<fs::path> &output_override, const ResolvedPipeline &resolved_pipeline) {
        if (source) {
            auto [input, output] = validate_io_paths(source.value(), output_override.value_or(
                default_output_path(*source)
            ));
            auto input_data = read_file(input);
            return {std::move(input_data), output};
        }
        auto input_data = build_canvas(resolved_pipeline);
        auto output = output_override.value_or(resolved_pipeline.canvas->file_name);
        log(LogLevel::info, "Canvas generated successfully");
        return {std::move(input_data), output};

    }

    std::pair<FileData, fs::path> validate_io(const std::optional<fs::path> &source, const std::optional<fs::path> &output_override, const ResolvedPipeline &resolved_pipeline, bool overwrite) {
        auto [input_date, output] = resolve_input_data_and_output(source, output_override, resolved_pipeline);
        if (!should_write_output(output, overwrite)) {
            log(LogLevel::info, "Output file was not overwritten");
            throw std::logic_error("Please provide a valid output file or allow overwrite");
        }
        return {std::move(input_date), output};
    }
}

void process_pipeline(
    const std::vector<PipelineStage> &steps,
    bool overwrite,
    bool debug,
    const std::optional<fs::path> &source,
    const std::optional<fs::path> &output_override,
    const MacroMap &cli_macros
) {
    ResolvedPipeline resolved_pipeline =
        resolve_pipeline_steps(steps, source.has_value(), cli_macros);

    auto [input, output] = validate_io(source, output_override, resolved_pipeline, overwrite);

    MagritteRunOptions options{
        .file_data = std::move(input),
        .steps = std::move(resolved_pipeline.steps),
        .macros = std::move(resolved_pipeline.macros),
        .overwrite = overwrite,
        .debug = debug,
    };
    auto data = process_image(options);

    save_file(output, data);
    std::clog << "\nFile saved to " + output.filename().string() << '\n';
}
