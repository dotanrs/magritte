#include "../common/test_support.h"
#include "magritte/steps/warp_formula.h"

void test_warp_formula() {
const auto image = [] {
    return FileData{
        .width = 2,
        .height = 2,
        .pixels = {
            Pixel{.red = 0, .green = 10, .blue = 20, .alpha = 0},
            Pixel{.red = 100, .green = 30, .blue = 40, .alpha = 40},
            Pixel{.red = 200, .green = 50, .blue = 60, .alpha = 80},
            Pixel{.red = 240, .green = 70, .blue = 80, .alpha = 120},
        },
    };
};

const FileData identity = warp_formula_step().apply(
    image(),
    {"(X, Y)"}
);
expect(
    identity.width == 2 &&
    identity.height == 2 &&
    identity.pixels[0].red == 0 &&
    identity.pixels[0].alpha == 0 &&
    identity.pixels[3].red == 240 &&
    identity.pixels[3].alpha == 120,
    "identity warp should preserve dimensions and pixels"
);

const FileData interpolated = warp_formula_step().apply(
    image(),
    {"(0.5, 0.5)"}
);
expect(
    interpolated.pixels[0].red == 135 &&
    interpolated.pixels[0].green == 40 &&
    interpolated.pixels[0].blue == 50 &&
    interpolated.pixels[0].alpha == 60 &&
    interpolated.pixels[3].red == 135 &&
    interpolated.pixels[3].alpha == 60,
    "warp should bilinearly interpolate RGB and alpha"
);

const FileData clamped = warp_formula_step().apply(
    image(),
    {"(-100, 100)"}
);
expect(
    clamped.pixels[0].red == 200 &&
    clamped.pixels[0].green == 50 &&
    clamped.pixels[0].blue == 60 &&
    clamped.pixels[0].alpha == 80 &&
    clamped.pixels[3].red == 200,
    "warp should clamp source coordinates to the image edges"
);
}

