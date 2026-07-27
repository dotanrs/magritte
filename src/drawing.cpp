#include "pixlie/drawing_config.h"

#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {
    fs::path default_output_path(const fs::path &input) {
        return input.parent_path() /
               (input.stem().string() + "_copy" + input.extension().string());
    }
}

void process_drawing(
    const fs::path &path,
    bool overwrite,
    bool debug,
    const std::optional<fs::path> &source_override
) {
    DrawingConfig config = load_drawing_config(path);
    if (source_override) {
        config.canvas.reset();
        config.source_image =
            fs::absolute(*source_override).lexically_normal();
    }
    if (config.source_image) {
        Options options{
            .input = *config.source_image,
            .output = default_output_path(*config.source_image),
            .processors = std::move(config.processors),
            .overwrite = overwrite,
            .debug = debug,
        };
        process_image(options);
        return;
    }

    const CanvasConfig &canvas_config = *config.canvas;
    FileData canvas{
        .width = canvas_config.width,
        .height = canvas_config.height,
        .pixels = std::vector<Pixel>(
            canvas_config.width * canvas_config.height,
            Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
        ),
    };
    process_created_image(
        canvas_config.file_name,
        std::move(canvas),
        config.processors,
        overwrite,
        debug
    );
}
