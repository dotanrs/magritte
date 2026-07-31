#include <optional>

#include "../common/test_support.h"
#include "magritte/steps/local_warp.h"

void test_local_warp_formula() {
const auto image = [] {
    return FileData{
        .width = 3,
        .height = 1,
        .pixels = {
            Pixel{.red = 0, .green = 10, .blue = 20, .alpha = 30},
            Pixel{.red = 1, .green = 30, .blue = 40, .alpha = 50},
            Pixel{.red = 2, .green = 50, .blue = 60, .alpha = 70},
        },
    };
};

const FileData selected = local_warp_step().apply(
    image(),
    {"(red(0, 0), Y)"}
);
expect(
    selected.pixels[0].red == 0 &&
    selected.pixels[0].alpha == 30 &&
    selected.pixels[1].red == 1 &&
    selected.pixels[1].alpha == 50 &&
    selected.pixels[2].red == 2 &&
    selected.pixels[2].alpha == 70,
    "local-warp should derive source coordinates from source colors"
);

const FileData neighboring = local_warp_step().apply(
    image(),
    {"(green(1, 0) / 20 - 0.5, Y)"}
);
expect(
    neighboring.pixels[0].red == 1 &&
    neighboring.pixels[0].green == 30 &&
    neighboring.pixels[0].alpha == 50 &&
    neighboring.pixels[1].red == 2 &&
    neighboring.pixels[1].green == 50 &&
    neighboring.pixels[1].alpha == 70 &&
    neighboring.pixels[2].red == 2 &&
    neighboring.pixels[2].alpha == 70,
    "local-warp should sample neighboring colors and clamp image edges"
);
}
