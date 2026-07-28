#include "../common/test_support.h"
#include "pixlie/processors/contrast.h"

void test_contrast() {
    FileData image{
        .width = 3,
        .height = 1,
        .pixels = {
            Pixel{.red = 64, .green = 96, .blue = 127, .alpha = 10},
            Pixel{.red = 128, .green = 160, .blue = 192, .alpha = 120},
            Pixel{.red = 0, .green = 255, .blue = 224, .alpha = 240},
        },
    };

    const FileData contrasted = contrast_processor().apply(image, {"2"});
    expect(
        contrasted.pixels[0].red == 0 &&
        contrasted.pixels[0].green == 64 &&
        contrasted.pixels[0].blue == 126 &&
        contrasted.pixels[1].red == 128 &&
        contrasted.pixels[1].green == 192 &&
        contrasted.pixels[1].blue == 255 &&
        contrasted.pixels[2].red == 0 &&
        contrasted.pixels[2].green == 255 &&
        contrasted.pixels[2].blue == 255,
        "contrast should scale RGB values away from the midpoint and clamp"
    );
    expect(
        contrasted.pixels[0].alpha == 10 &&
        contrasted.pixels[1].alpha == 120 &&
        contrasted.pixels[2].alpha == 240,
        "contrast should preserve alpha"
    );

    const FileData identity = contrast_processor().apply(image, {"1"});
    expect(
        red_values(identity) == red_values(image),
        "a contrast factor of one should preserve the image"
    );
}
