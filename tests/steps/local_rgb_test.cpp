#include "../common/test_support.h"
#include "magritte/steps/local_rgb.h"

void test_local_rgb_formula() {
const auto image = [] {
    return FileData{
        .width = 3,
        .height = 1,
        .pixels = {
            Pixel{.red = 10, .green = 1, .blue = 100, .alpha = 40},
            Pixel{.red = 20, .green = 2, .blue = 110, .alpha = 80},
            Pixel{.red = 30, .green = 3, .blue = 120, .alpha = 120},
        },
    };
};

const FileData neighboring = local_rgb_step().apply(
    image(),
    {"(red(-1, 0), green(1, 0), blue(0, 0))"}
);
expect(
    neighboring.pixels[0].red == 10 &&
    neighboring.pixels[0].green == 2 &&
    neighboring.pixels[0].blue == 100 &&
    neighboring.pixels[0].alpha == 40 &&
    neighboring.pixels[1].red == 10 &&
    neighboring.pixels[1].green == 3 &&
    neighboring.pixels[1].blue == 110 &&
    neighboring.pixels[1].alpha == 80 &&
    neighboring.pixels[2].red == 20 &&
    neighboring.pixels[2].green == 3 &&
    neighboring.pixels[2].blue == 120 &&
    neighboring.pixels[2].alpha == 120,
    "local-rgb should sample the immutable source and clamp at edges"
);

const FileData interpolated = local_rgb_step().apply(
    image(),
    {
        "("
        "red(0.5, 0), "
        "green(-0.5, 0) * 10, "
        "blue(sin(PI / 2), 0)"
        ")"
    }
);
expect(
    interpolated.pixels[0].red == 15 &&
    interpolated.pixels[0].green == 10 &&
    interpolated.pixels[0].blue == 110 &&
    interpolated.pixels[1].red == 25 &&
    interpolated.pixels[1].green == 15 &&
    interpolated.pixels[1].blue == 120 &&
    interpolated.pixels[2].red == 30 &&
    interpolated.pixels[2].green == 25 &&
    interpolated.pixels[2].blue == 120,
    "local-rgb should bilinearly sample formula-defined offsets"
);
}

