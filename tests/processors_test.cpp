#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "pixlie/parser.h"
#include "pixlie/processors/blue_formula.h"
#include "pixlie/processors/color_swap.h"
#include "pixlie/processors/green_formula.h"
#include "pixlie/processors/red_formula.h"
#include "pixlie/processors/rotate.h"
#include "pixlie/processors/saturation_formula.h"

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::vector<std::uint8_t> red_values(const FileData& data) {
    std::vector<std::uint8_t> values;
    values.reserve(data.pixels.size());
    for (const Pixel& pixel : data.pixels) {
        values.push_back(pixel.red);
    }
    return values;
}

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

void test_red_formula_and_clamping() {
    FileData image{
        .width = 3,
        .height = 1,
        .pixels = {
            Pixel{.red = 100, .green = 50, .blue = 7, .alpha = 255},
            Pixel{.red = 10, .green = 50, .blue = 8, .alpha = 255},
            Pixel{.red = 200, .green = 0, .blue = 9, .alpha = 255},
        },
    };

    const FileData processed = red_formula_processor().apply(
        std::move(image),
        {"r * 2 - g"}
    );

    expect(
        red_values(processed) == std::vector<std::uint8_t>{150, 0, 255},
        "red formula should evaluate and clamp into [0, 255]"
    );
    expect(
        processed.pixels[0].green == 50 && processed.pixels[0].blue == 7,
        "red formula should not modify green or blue"
    );
}

void test_green_formula() {
    FileData image{
        .width = 1,
        .height = 1,
        .pixels = {
            Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 255},
        },
    };

    const FileData processed = green_formula_processor().apply(
        std::move(image),
        {"b * 2"}
    );

    expect(processed.pixels[0].green == 60, "green formula should update green");
    expect(
        processed.pixels[0].red == 10 && processed.pixels[0].blue == 30,
        "green formula should not modify red or blue"
    );
}

void test_blue_formula() {
    FileData image{
        .width = 1,
        .height = 1,
        .pixels = {
            Pixel{.red = 100, .green = 40, .blue = 30, .alpha = 255},
        },
    };

    const FileData processed = blue_formula_processor().apply(
        std::move(image),
        {"r - g"}
    );

    expect(processed.pixels[0].blue == 60, "blue formula should update blue");
    expect(
        processed.pixels[0].red == 100 && processed.pixels[0].green == 40,
        "blue formula should not modify red or green"
    );
}

void test_saturation_formula() {
    FileData image{
        .width = 3,
        .height = 1,
        .pixels = {
            Pixel{.red = 191, .green = 64, .blue = 64, .alpha = 40},
            Pixel{.red = 64, .green = 191, .blue = 64, .alpha = 80},
            Pixel{.red = 100, .green = 100, .blue = 100, .alpha = 120},
        },
    };

    const FileData processed = saturation_formula_processor().apply(
        std::move(image),
        {"s * 2"}
    );

    expect(
        processed.pixels[0].red == 255 &&
            processed.pixels[0].green == 0 &&
            processed.pixels[0].blue == 0 &&
            processed.pixels[0].alpha == 40,
        "saturation formula should increase saturation and preserve alpha"
    );
    expect(
        processed.pixels[1].red == 0 &&
            processed.pixels[1].green == 255 &&
            processed.pixels[1].blue == 0 &&
            processed.pixels[1].alpha == 80,
        "saturation formula should preserve hue and lightness"
    );
    expect(
        processed.pixels[2].red == 100 &&
            processed.pixels[2].green == 100 &&
            processed.pixels[2].blue == 100 &&
            processed.pixels[2].alpha == 120,
        "saturation formula should leave grayscale pixels unchanged"
    );
}

void test_color_swap() {
    FileData image{
        .width = 2,
        .height = 1,
        .pixels = {
            Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40},
            Pixel{.red = 50, .green = 60, .blue = 70, .alpha = 80},
        },
    };

    const FileData processed = color_swap_processor().apply(
        std::move(image),
        {"r", "b"}
    );

    expect(
        processed.pixels[0].red == 30 &&
            processed.pixels[0].green == 20 &&
            processed.pixels[0].blue == 10 &&
            processed.pixels[0].alpha == 40,
        "color swap should exchange the selected channels"
    );
    expect(
        processed.pixels[1].red == 70 &&
            processed.pixels[1].green == 60 &&
            processed.pixels[1].blue == 50 &&
            processed.pixels[1].alpha == 80,
        "color swap should process every pixel without changing alpha"
    );
}

void test_command_parser() {
    std::string error_message;

    expect(
        parse_processor_command("rotate -1").has_value(),
        "parser should accept a valid rotate command"
    );
    expect(
        parse_processor_command("r = (R + G) / 2").has_value(),
        "parser should accept a valid red formula"
    );
    expect(
        parse_processor_command("g = B * 2").has_value(),
        "parser should accept a valid green formula"
    );
    expect(
        parse_processor_command("b = R - G").has_value(),
        "parser should accept a valid blue formula"
    );
    expect(
        parse_processor_command("s = S * 2").has_value(),
        "parser should accept a valid saturation formula"
    );
    expect(
        parse_processor_command("r <-> b").has_value(),
        "parser should accept a valid color swap"
    );
    expect(
        !parse_processor_command("r <-> y", &error_message).has_value(),
        "parser should reject an invalid color swap channel"
    );
    expect(
        error_message == "color swap channels must be r, g, or b",
        "parser should describe an invalid color swap channel"
    );
    error_message.clear();
    expect(
        !parse_processor_command("rotate nope", &error_message).has_value(),
        "parser should reject a non-integer rotation"
    );
    expect(
        !error_message.empty(),
        "parser should return an error message for an invalid command"
    );
    error_message.clear();
    expect(
        !parse_processor_command("contrast 2", &error_message).has_value(),
        "parser should reject an unknown processor"
    );
    expect(
        error_message == "unknown processor",
        "parser should describe an unknown processor"
    );
}

} // namespace

int main() {
    test_rotation();
    test_red_formula_and_clamping();
    test_green_formula();
    test_blue_formula();
    test_saturation_formula();
    test_color_swap();
    test_command_parser();

    if (failures == 0) {
        std::cout << "All processor tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
