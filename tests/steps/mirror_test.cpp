#include "../common/test_support.h"
#include "magritte/steps/mirror.h"

void test_mirror() {
const auto image = [] {
    FileData result{
        .width = 3,
        .height = 2,
        .pixels = std::vector<Pixel>(
            6,
            Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
        ),
    };
    for (std::size_t index = 0; index < result.pixels.size(); ++index) {
        result.pixels[index].red =
            static_cast<std::uint8_t>(index + 1);
    }
    return result;
};

const FileData across_x = mirror_step().apply(image(), {"y"});
expect(
    red_values(across_x) ==
    std::vector<std::uint8_t>{4, 5, 6, 1, 2, 3},
    "mirror y should exchange the top and bottom"
);

const FileData across_y = mirror_step().apply(image(), {"x"});
expect(
    red_values(across_y) ==
    std::vector<std::uint8_t>{3, 2, 1, 6, 5, 4},
    "mirror x should exchange the left and right"
);
}

