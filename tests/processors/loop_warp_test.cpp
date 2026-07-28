#include "../common/test_support.h"
#include "magritte/processors/loop_warp.h"

void test_loop_warp() {
    const auto image = [] {
        return FileData{
            .width = 4,
            .height = 1,
            .pixels = {
                Pixel{.red = 10, .green = 1, .blue = 2, .alpha = 20},
                Pixel{.red = 20, .green = 3, .blue = 4, .alpha = 40},
                Pixel{.red = 30, .green = 5, .blue = 6, .alpha = 60},
                Pixel{.red = 40, .green = 7, .blue = 8, .alpha = 80},
            },
        };
    };

    const FileData twice = loop_warp_processor().apply(
        image(),
        {"2", "(X - 1, Y)"}
    );
    expect(
        red_values(twice) == std::vector<std::uint8_t>{10, 10, 10, 20} &&
        twice.pixels[0].alpha == 20 &&
        twice.pixels[3].alpha == 40,
        "loop-warp should feed each warp result into the next iteration"
    );

    const FileData zero_times = loop_warp_processor().apply(
        image(),
        {"0", "(X - 1, Y)"}
    );
    expect(
        red_values(zero_times) ==
        std::vector<std::uint8_t>{10, 20, 30, 40},
        "zero loop iterations should preserve the image"
    );
}
