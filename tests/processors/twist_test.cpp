#include <cmath>
#include <string>

#include "../common/test_support.h"
#include "pixlie/processors/twist.h"

void test_twist() {
    FileData image = blank_image(101, 101);
    image.pixels[100 * image.width + 50] =
        Pixel{.red = 210, .green = 80, .blue = 30, .alpha = 120};

    const FileData twisted = twist_processor().apply(
        image,
        {"50", "50", std::to_string(std::acos(-1.0))}
    );
    const Pixel &quarter_turn_sample =
        twisted.pixels[50 * twisted.width + 100];
    expect(
        quarter_turn_sample.red == 210 &&
        quarter_turn_sample.green == 80 &&
        quarter_turn_sample.blue == 30 &&
        quarter_turn_sample.alpha == 120,
        "twist force should rotate increasingly distant source coordinates"
    );

    const FileData reversed = twist_processor().apply(
        image,
        {"50", "50", std::to_string(-std::acos(-1.0))}
    );
    expect(
        reversed.pixels[50 * reversed.width].red == 210,
        "negative twist force should reverse the rotation direction"
    );

    const FileData identity = twist_processor().apply(
        image,
        {"25", "75", "0"}
    );
    expect(
        red_values(identity) == red_values(image),
        "zero twist force should preserve the image"
    );
}
