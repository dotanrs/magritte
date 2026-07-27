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

    FileData localized_source = blank_image(101, 101);
    localized_source.pixels[50 * localized_source.width + 60].red = 190;
    localized_source.pixels[50 * localized_source.width + 90].red = 80;
    const FileData localized = twist_processor().apply(
        localized_source,
        {"50", "50", "4", "20"}
    );
    expect(
        red_values(localized) != red_values(localized_source),
        "twist radius should apply a smooth local rotation"
    );
    expect(
        localized.pixels[50 * localized.width + 90].red == 80,
        "twist radius should preserve pixels outside its boundary"
    );
}

void test_twist_debug_hints() {
    FileData image{
        .width = 101,
        .height = 101,
        .pixels = std::vector<Pixel>(
            101 * 101,
            Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 120}
        ),
    };
    const std::vector<std::string> arguments{"50", "50", "4", "20"};
    const FileData processed = twist_processor().apply(image, arguments);
    const FileData debugged = twist_processor().add_debug_hints(
        twist_processor().apply(std::move(image), arguments),
        arguments
    );

    const Pixel &center = debugged.pixels[50 * debugged.width + 50];
    expect(
        center.red == 255 &&
        center.green == 0 &&
        center.blue == 255 &&
        center.alpha == processed.pixels[50 * processed.width + 50].alpha,
        "twist debug hints should mark the center without changing alpha"
    );

    const Pixel &boundary = debugged.pixels[30 * debugged.width + 50];
    expect(
        boundary.red == 255 &&
        boundary.green == 215 &&
        boundary.blue == 0 &&
        boundary.alpha == processed.pixels[30 * processed.width + 50].alpha,
        "twist debug hints should mark the radius boundary"
    );

    bool found_spin_line = false;
    for (const Pixel &pixel: debugged.pixels) {
        if (pixel.red == 0 && pixel.green == 220 && pixel.blue == 255) {
            found_spin_line = true;
            expect(
                pixel.alpha == 120,
                "twist spin guide should preserve alpha"
            );
            break;
        }
    }
    expect(
        found_spin_line,
        "twist debug hints should draw a force-shaped spin line"
    );
}
