#include "../common/test_support.h"
#include "pixlie/processors/fisheye.h"
#include "pixlie/processors/rotate.h"

void test_fisheye() {
const auto image = [] {
    FileData result{
        .width = 5,
        .height = 5,
        .pixels = std::vector<Pixel>(
            25,
            Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
        ),
    };
    for (std::size_t y = 0; y < result.height; ++y) {
        for (std::size_t x = 0; x < result.width; ++x) {
            Pixel &pixel = result.pixels[y * result.width + x];
            pixel.red = static_cast<std::uint8_t>(x * 60);
            pixel.green = static_cast<std::uint8_t>(y * 60);
        }
    }
    return result;
};
const auto middle_red = [](const FileData &data) {
    std::vector<std::uint8_t> values;
    for (std::size_t x = 0; x < data.width; ++x) {
        values.push_back(data.pixels[2 * data.width + x].red);
    }
    return values;
};

const FileData enlarged = fisheye_processor().apply(
    image(),
    {"50", "50", "1", "40"}
);
expect(
    middle_red(enlarged) ==
    std::vector<std::uint8_t>{0, 80, 120, 160, 240},
    "positive fisheye amount should enlarge around its center"
);
expect(
    enlarged.pixels[4].red == 240 &&
    enlarged.pixels[4].green == 0,
    "fisheye should preserve pixels outside its radius"
);

const FileData shrunk = fisheye_processor().apply(
    image(),
    {"50", "50", "-0.5", "40"}
);
expect(
    middle_red(shrunk) ==
    std::vector<std::uint8_t>{0, 40, 120, 200, 240},
    "negative fisheye amount should shrink around its center"
);

const FileData offset = fisheye_processor().apply(
    image(),
    {"25", "50", "1", "60"}
);
expect(
    middle_red(offset) ==
    std::vector<std::uint8_t>{24, 60, 96, 150, 240},
    "fisheye should use the supplied x and y as its center"
);

const FileData identity = fisheye_processor().apply(
    image(),
    {"50", "50", "0"}
);
expect(
    red_values(identity) == red_values(image()),
    "zero fisheye amount should preserve the image"
);

const FileData default_radius = fisheye_processor().apply(
    image(),
    {"50", "50", "1"}
);
const FileData explicit_default_radius = fisheye_processor().apply(
    image(),
    {"50", "50", "1", "100"}
);
expect(
    red_values(default_radius) == red_values(explicit_default_radius),
    "omitted fisheye radius should default to 100 percent"
);
}

void test_fisheye_debug_hints() {
FileData image{
    .width = 11,
    .height = 11,
    .pixels = std::vector<Pixel>(
        121,
        Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 200}
    ),
};
image.pixels[5 * image.width + 5].alpha = 40;
image.pixels[2 * image.width + 5].alpha = 80;

const std::vector<std::string> arguments{"50", "50", "1", "30"};
const FileData processed = fisheye_processor().apply(image, arguments);
const FileData debugged = fisheye_processor().add_debug_hints(
    fisheye_processor().apply(std::move(image), arguments),
    arguments
);

const Pixel &center = debugged.pixels[5 * debugged.width + 5];
expect(
    center.red == 255 &&
    center.green == 0 &&
    center.blue == 255 &&
    center.alpha == processed.pixels[5 * processed.width + 5].alpha,
    "fisheye debug hints should mark the center without changing alpha"
);

const Pixel &boundary = debugged.pixels[2 * debugged.width + 5];
expect(
    boundary.red == 255 &&
    boundary.green == 215 &&
    boundary.blue == 0 &&
    boundary.alpha == processed.pixels[2 * processed.width + 5].alpha,
    "fisheye debug hints should mark the radius boundary"
);

const Pixel &outside = debugged.pixels.front();
const Pixel &processed_outside = processed.pixels.front();
expect(
    outside.red == processed_outside.red &&
    outside.green == processed_outside.green &&
    outside.blue == processed_outside.blue &&
    outside.alpha == processed_outside.alpha,
    "fisheye debug hints should leave pixels away from the guide unchanged"
);

const FileData other_processor = rotate_processor().add_debug_hints(
    blank_image(2, 2),
    {"1"}
);
expect(
    red_values(other_processor) ==
    std::vector<std::uint8_t>{0, 0, 0, 0},
    "processors without debug hints should leave the image unchanged"
);
}
