#include "../common/test_support.h"
#include "magritte/processors/saturation_formula.h"

void test_saturation_formula() {
FileData image{
    .width = 3,
    .height = 1,
    .pixels = {
        Pixel{.red = 191, .green = 64, .blue = 64, .alpha = 40},
        Pixel{.red = 64, .green = 191, .blue = 64, .alpha = 80},
        Pixel{.red = 100, .green = 100, .blue = 100, .alpha = 120},
    },
};

const FileData processed = saturation_formula_processor().apply(
    std::move(image),
    {"s * 2"}
);

expect(
    processed.pixels[0].red == 255 &&
    processed.pixels[0].green == 0 &&
    processed.pixels[0].blue == 0 &&
    processed.pixels[0].alpha == 40,
    "saturation formula should increase saturation and preserve alpha"
);
expect(
    processed.pixels[1].red == 0 &&
    processed.pixels[1].green == 255 &&
    processed.pixels[1].blue == 0 &&
    processed.pixels[1].alpha == 80,
    "saturation formula should preserve hue and lightness"
);
expect(
    processed.pixels[2].red == 100 &&
    processed.pixels[2].green == 100 &&
    processed.pixels[2].blue == 100 &&
    processed.pixels[2].alpha == 120,
    "saturation formula should leave grayscale pixels unchanged"
);

FileData coordinate_image{
    .width = 3,
    .height = 1,
    .pixels = std::vector<Pixel>(
        3,
        Pixel{.red = 191, .green = 64, .blue = 64, .alpha = 255}
    ),
};
const FileData coordinate_processed =
        saturation_formula_processor().apply(
            std::move(coordinate_image),
            {"(U + 1) * 127.5"}
        );
expect(
    coordinate_processed.pixels[0].red == 128 &&
    coordinate_processed.pixels[0].green == 128 &&
    coordinate_processed.pixels[0].blue == 128 &&
    coordinate_processed.pixels[1].red == 191 &&
    coordinate_processed.pixels[1].green == 64 &&
    coordinate_processed.pixels[2].red == 255 &&
    coordinate_processed.pixels[2].green == 0,
    "saturation formulas should support coordinate variables"
);
}

