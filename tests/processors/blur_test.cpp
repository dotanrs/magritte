#include "../common/test_support.h"
#include "magritte/processors/blur.h"

void test_blur() {
FileData image{
    .width = 3,
    .height = 3,
    .pixels = std::vector<Pixel>(
        9,
        Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 40}
    ),
};
image.pixels[4] =
    Pixel{.red = 90, .green = 180, .blue = 45, .alpha = 200};

const FileData blurred = blur_processor().apply(
    std::move(image),
    {"1"}
);
expect(
    red_values(blurred) ==
    std::vector<std::uint8_t>{23, 15, 23, 15, 10, 15, 23, 15, 23},
    "blur should average the available square neighborhood"
);
expect(
    blurred.pixels[0].green == 45 &&
    blurred.pixels[4].green == 20 &&
    blurred.pixels[0].blue == 11 &&
    blurred.pixels[4].blue == 5,
    "blur should process every RGB channel"
);
expect(
    blurred.pixels[0].alpha == 40 &&
    blurred.pixels[4].alpha == 200,
    "blur should preserve each pixel's alpha"
);
}

