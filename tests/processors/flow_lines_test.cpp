#include "../common/test_support.h"

#include "magritte/processors/flow_lines.h"

namespace {
    FileData white_image(std::size_t width, std::size_t height) {
        return FileData{
            .width = width,
            .height = height,
            .pixels = std::vector<Pixel>(
                width * height,
                Pixel{
                    .red = 255,
                    .green = 255,
                    .blue = 255,
                    .alpha = 173,
                }
            ),
        };
    }
}

void test_flow_lines() {
    const FileData horizontal = flow_lines_processor().apply(
        white_image(32, 32),
        {"8", "64", "1", "1", "#000000", "1", "(1, 0)"}
    );
    const Pixel &on_line = horizontal.pixels[4 * horizontal.width + 16];
    const Pixel &between_lines =
        horizontal.pixels[8 * horizontal.width + 16];
    expect(
        on_line.red == 0 &&
        on_line.green == 0 &&
        on_line.blue == 0,
        "flow-lines should trace a constant horizontal field"
    );
    expect(
        between_lines.red == 255 &&
        between_lines.green == 255 &&
        between_lines.blue == 255,
        "flow-lines should honor seed spacing"
    );
    expect(
        on_line.alpha == 173,
        "flow-lines should preserve destination alpha"
    );

    const FileData translucent = flow_lines_processor().apply(
        white_image(24, 24),
        {"8", "48", "1", "1", "#000000", "0.5", "(0, 1)"}
    );
    const Pixel &vertical_line =
        translucent.pixels[12 * translucent.width + 4];
    expect(
        vertical_line.red == 128 &&
        vertical_line.green == 128 &&
        vertical_line.blue == 128,
        "flow-lines should alpha composite its stroke color"
    );

    const FileData rotational = flow_lines_processor().apply(
        white_image(41, 41),
        {"10", "80", "1", "1.5", "#204080", "1", "(-V, U)"}
    );
    std::size_t changed = 0;
    for (const Pixel &pixel: rotational.pixels) {
        if (pixel.red != 255 || pixel.green != 255 || pixel.blue != 255) {
            ++changed;
        }
    }
    expect(
        changed > 100,
        "flow-lines should RK4-trace a rotational field"
    );
}
