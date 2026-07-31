#include <stdexcept>

#include "../common/test_support.h"
#include "magritte/steps/black_and_white.h"

void test_black_and_white() {
    FileData image{
        .width = 4,
        .height = 1,
        .pixels = {
            Pixel{.red = 255, .green = 0, .blue = 0, .alpha = 10},
            Pixel{.red = 0, .green = 255, .blue = 0, .alpha = 80},
            Pixel{.red = 0, .green = 0, .blue = 255, .alpha = 160},
            Pixel{.red = 100, .green = 150, .blue = 200, .alpha = 240},
        },
    };

    const FileData grayscale =
        black_and_white_step().apply(image, {"1"});
    expect(
        grayscale.pixels[0].red == 76 &&
        grayscale.pixels[1].red == 150 &&
        grayscale.pixels[2].red == 29 &&
        grayscale.pixels[3].red == 141,
        "black-and-white should use perceptual RGB luminance"
    );
    for (const Pixel &pixel: grayscale.pixels) {
        expect(
            pixel.red == pixel.green && pixel.green == pixel.blue,
            "black-and-white should write equal RGB channels"
        );
    }
    expect(
        grayscale.pixels[0].alpha == 10 &&
        grayscale.pixels[1].alpha == 80 &&
        grayscale.pixels[2].alpha == 160 &&
        grayscale.pixels[3].alpha == 240,
        "black-and-white should preserve alpha"
    );

    const FileData brightened =
        black_and_white_step().apply(image, {"1.2"});
    expect(
        brightened.pixels[3].red == 169 &&
        brightened.pixels[3].green == 169 &&
        brightened.pixels[3].blue == 169,
        "black-and-white should apply the brightness multiplier"
    );

    FileData white{
        .width = 1,
        .height = 1,
        .pixels = {
            Pixel{.red = 255, .green = 255, .blue = 255, .alpha = 200},
        },
    };
    const FileData clamped =
        black_and_white_step().apply(white, {"2"});
    expect(
        clamped.pixels[0].red == 255 &&
        clamped.pixels[0].green == 255 &&
        clamped.pixels[0].blue == 255,
        "black-and-white should clamp brightened luminance to white"
    );

    const FileData black =
        black_and_white_step().apply(image, {"0"});
    expect(
        black.pixels[0].red == 0 &&
        black.pixels[1].red == 0 &&
        black.pixels[2].red == 0 &&
        black.pixels[3].red == 0,
        "a zero brightness multiplier should produce black"
    );

    const auto arguments =
        black_and_white_step().parse_arguments("black-and-white 1.25");
    expect(
        arguments ==
        std::optional<std::vector<std::string>>{{"1.25"}},
        "black-and-white should recognize its command and multiplier"
    );
    expect(
        !black_and_white_step().parse_arguments("contrast 2").has_value(),
        "black-and-white should decline another step's command"
    );

    bool rejected_negative_multiplier = false;
    try {
        black_and_white_step().validate({"-0.5"});
    } catch (const std::invalid_argument &) {
        rejected_negative_multiplier = true;
    }
    expect(
        rejected_negative_multiplier,
        "black-and-white should reject a negative brightness multiplier"
    );
}
