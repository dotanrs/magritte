#include "../common/test_support.h"
#include "magritte/processors/rotate.h"

void test_rotation() {
const Pixel pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255};
FileData image{
    .width = 2,
    .height = 3,
    .pixels = std::vector<Pixel>(6, pixel),
};
for (std::size_t index = 0; index < image.pixels.size(); ++index) {
    image.pixels[index].red = static_cast<std::uint8_t>(index + 1);
}

const FileData rotated = rotate_processor().apply(
    std::move(image),
    {"1"}
);

expect(rotated.width == 3, "90-degree rotation should update width");
expect(rotated.height == 2, "90-degree rotation should update height");
expect(
    red_values(rotated) == std::vector<std::uint8_t>{2, 4, 6, 1, 3, 5},
    "90-degree rotation should map pixels clockwise"
);
}

