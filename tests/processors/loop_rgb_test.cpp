#include "../common/test_support.h"
#include "pixlie/processors/loop_rgb.h"

void test_loop_rgb() {
    const FileData cycled = loop_rgb_processor().apply(
        FileData{
            .width = 1,
            .height = 1,
            .pixels = {
                Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40}
            },
        },
        {"2", "(G, B, R)"}
    );
    expect(
        loop_rgb_processor().name() == "loop-rgb" &&
        cycled.pixels[0].red == 30 &&
        cycled.pixels[0].green == 10 &&
        cycled.pixels[0].blue == 20 &&
        cycled.pixels[0].alpha == 40,
        "loop-rgb should feed each RGB result into the next iteration"
    );

    FileData image{
        .width = 4,
        .height = 1,
        .pixels = {
            Pixel{.red = 10, .green = 1, .blue = 2, .alpha = 20},
            Pixel{.red = 20, .green = 3, .blue = 4, .alpha = 40},
            Pixel{.red = 30, .green = 5, .blue = 6, .alpha = 60},
            Pixel{.red = 40, .green = 7, .blue = 8, .alpha = 80},
        },
    };
    const FileData zero_iterations = loop_rgb_processor().apply(
        std::move(image),
        {"0", "(G, B, R)"}
    );
    expect(
        red_values(zero_iterations) ==
        std::vector<std::uint8_t>{10, 20, 30, 40},
        "zero loop-rgb iterations should preserve the image"
    );
}
