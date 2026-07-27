#include <sstream>
#include <stdexcept>
#include <string>

#include "common/test_support.h"
#include "pixlie/drawing_config.h"

namespace {
    void expect_invalid(std::string_view yaml, std::string_view message) {
        std::istringstream input{std::string(yaml)};
        try {
            static_cast<void>(parse_drawing_config(input, "test.yml"));
            expect(false, std::string(message));
        } catch (const std::invalid_argument &) {
            expect(true, std::string(message));
        }
    }
}

void test_drawing_config() {
    std::istringstream input(R"YAML(
# An original generative drawing
canvas:
  file_name: "output/field.jpg"
  width: 640
  height: 480
processors:
  - name: background
    command: 'rgb = (245, 241, 230)'
  - name: blue field
    command: "rgb = (R, 80 + 60 * sin(D / 8), 180)"
)YAML");

    const DrawingConfig config = parse_drawing_config(input, "drawing.yml");
    expect(config.canvas.has_value(), "drawing has canvas");
    expect(!config.source_image.has_value(), "drawing has no source image");
    expect(
        config.canvas->file_name == "output/field.jpg",
        "drawing output path"
    );
    expect(config.canvas->width == 640, "drawing canvas width");
    expect(config.canvas->height == 480, "drawing canvas height");
    expect(config.processors.size() == 2, "drawing processor count");
    expect(
        config.processors[1].name == "blue field",
        "drawing processor name"
    );
    expect(
        config.processors[1].command ==
            "rgb = (R, 80 + 60 * sin(D / 8), 180)",
        "drawing processor command"
    );

    expect_invalid(
        "canvas:\n  file_name: x.jpg\n  width: 0\n  height: 5\n"
        "processors:\n",
        "drawing rejects zero dimensions"
    );
    expect_invalid(
        "canvas:\n  file_name: x.png\n  width: 5\n  height: 5\n"
        "processors:\n",
        "drawing rejects non-JPEG output"
    );
    expect_invalid(
        "canvas:\n  file_name: x.jpg\n  width: 5\n  height: 5\n"
        "processors:\n  - name: incomplete\n",
        "drawing requires each processor command"
    );

    std::istringstream scalar_source(R"YAML(
source_image: "input/photo.jpg"
processors:
  - name: soften
    command: "blur 2"
)YAML");
    const DrawingConfig source_config =
        parse_drawing_config(scalar_source, "source.yml");
    expect(!source_config.canvas.has_value(), "source drawing has no canvas");
    expect(
        source_config.source_image == "input/photo.jpg",
        "scalar source_image path"
    );

    std::istringstream mapped_source(R"YAML(
source_image:
  file_name: input/photo.jpg
processors:
)YAML");
    const DrawingConfig mapped_config =
        parse_drawing_config(mapped_source, "mapped.yml");
    expect(
        mapped_config.source_image == "input/photo.jpg",
        "mapped source_image path"
    );

    expect_invalid(
        "canvas:\n"
        "  file_name: output.jpg\n"
        "  width: 5\n"
        "  height: 5\n"
        "source_image: input.jpg\n"
        "processors:\n",
        "drawing rejects canvas and source_image together"
    );
    expect_invalid(
        "source_image:\nprocessors:\n",
        "drawing requires source_image file_name"
    );
}
