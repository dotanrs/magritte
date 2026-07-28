#include "../common/test_support.h"
#include "magritte/processors/spin.h"

void test_spin() {
    FileData image = blank_image(101, 101);
    image.pixels[100 * image.width + 50] =
        Pixel{.red = 210, .green = 80, .blue = 30, .alpha = 120};

    const FileData spun = spin_processor().apply(
        image,
        {"50", "50", "90"}
    );
    const Pixel &quarter_turn_sample = spun.pixels[50 * spun.width + 100];
    expect(
        quarter_turn_sample.red == 210 &&
        quarter_turn_sample.green == 80 &&
        quarter_turn_sample.blue == 30 &&
        quarter_turn_sample.alpha == 120,
        "spin angle should rotate every source coordinate equally"
    );

    const FileData identity = spin_processor().apply(
        image,
        {"25", "75", "0", "20"}
    );
    expect(
        red_values(identity) == red_values(image),
        "zero spin angle should preserve the image"
    );

    FileData localized_source = blank_image(101, 101);
    localized_source.pixels[60 * localized_source.width + 50].red = 190;
    localized_source.pixels[50 * localized_source.width + 90].red = 80;
    const FileData localized = spin_processor().apply(
        localized_source,
        {"50", "50", "90", "20"}
    );
    expect(
        localized.pixels[50 * localized.width + 60].red == 190,
        "spin radius should rotate pixels inside its boundary"
    );
    expect(
        localized.pixels[50 * localized.width + 90].red == 80,
        "spin radius should preserve pixels outside its boundary"
    );
}

void test_spin_debug_hints() {
    FileData image{
        .width = 101,
        .height = 101,
        .pixels = std::vector<Pixel>(
            101 * 101,
            Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 120}
        ),
    };
    const std::vector<std::string> arguments{"50", "50", "90", "20"};
    const FileData debugged = spin_processor().add_debug_hints(
        spin_processor().apply(std::move(image), arguments),
        arguments
    );

    const Pixel &center = debugged.pixels[50 * debugged.width + 50];
    expect(
        center.red == 255 &&
        center.green == 0 &&
        center.blue == 255 &&
        center.alpha == 120,
        "spin debug hints should mark the center without changing alpha"
    );
    const Pixel &boundary = debugged.pixels[50 * debugged.width + 30];
    expect(
        boundary.red == 255 &&
        boundary.green == 215 &&
        boundary.blue == 0 &&
        boundary.alpha == 120,
        "spin debug hints should mark the radius boundary"
    );
    const Pixel &angle_line = debugged.pixels[65 * debugged.width + 50];
    expect(
        angle_line.red == 0 &&
        angle_line.green == 220 &&
        angle_line.blue == 255 &&
        angle_line.alpha == 120,
        "spin debug hints should draw the supplied angle"
    );
}
