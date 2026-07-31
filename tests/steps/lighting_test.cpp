#include "../common/test_support.h"
#include "magritte/steps/lighting.h"

void test_lighting() {
const Pixel dark{
    .red = 10,
    .green = 10,
    .blue = 10,
    .alpha = 40,
};
FileData image{
    .width = 5,
    .height = 1,
    .pixels = std::vector<Pixel>(5, dark),
};
image.pixels[1] =
    Pixel{.red = 100, .green = 100, .blue = 100, .alpha = 80};
image.pixels[3] =
    Pixel{.red = 150, .green = 150, .blue = 150, .alpha = 120};

const FileData from_left = lighting_step().apply(
    image,
    {"180", "#FF0000", "50", "1", "0", "0"}
);
expect(
    from_left.pixels[0].red == 10 &&
    from_left.pixels[0].green == 10 &&
    from_left.pixels[0].blue == 10,
    "lighting should leave empty dark regions alone when atmosphere is zero"
);
expect(
    from_left.pixels[1].red > image.pixels[1].red &&
    from_left.pixels[1].red > from_left.pixels[3].red &&
    from_left.pixels[1].green < image.pixels[1].green,
    "lighting should tint the source-facing surface more than an occluded one"
);
expect(
    from_left.pixels[0].alpha == 40 &&
    from_left.pixels[1].alpha == 80 &&
    from_left.pixels[3].alpha == 120,
    "lighting should preserve alpha"
);

FileData atmosphere_image{
    .width = 3,
    .height = 1,
    .pixels = std::vector<Pixel>(3, dark),
};
const FileData atmosphere = lighting_step().apply(
    std::move(atmosphere_image),
    {"0", "#0000FF", "200", "1", "0", "0.5"}
);
expect(
    atmosphere.pixels[2].blue > atmosphere.pixels[0].blue &&
    atmosphere.pixels[0].blue > dark.blue,
    "atmosphere should make a smooth color wash toward the light source"
);

const FileData no_strength = lighting_step().apply(
    image,
    {"golden-hour", "0"}
);
expect(
    red_values(no_strength) == red_values(image) &&
    no_strength.pixels[1].green == image.pixels[1].green &&
    no_strength.pixels[3].blue == image.pixels[3].blue,
    "a preset strength of zero should leave the image unchanged"
);

const FileData synthwave = lighting_step().apply(
    image,
    {"synthwave"}
);
expect(
    (
        synthwave.pixels[0].red != image.pixels[0].red ||
        synthwave.pixels[0].blue != image.pixels[0].blue
    ) &&
    synthwave.pixels[0].alpha == image.pixels[0].alpha,
    "multi-light presets should transform RGB while preserving alpha"
);
}

