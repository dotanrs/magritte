#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "pixlie/parser.h"
#include "pixlie/processors/red_formula.h"
#include "pixlie/processors/rotate.h"

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

void test_command_parser() {
    expect(
        parse_processor_command("rotate -1").has_value(),
        "parser should accept a valid rotate command"
    );
    expect(
        parse_processor_command("r = (R + G) / 2").has_value(),
        "parser should accept a valid red formula"
    );
    expect(
        !parse_processor_command("rotate nope").has_value(),
        "parser should reject a non-integer rotation"
    );
    expect(
        !parse_processor_command("contrast 2").has_value(),
        "parser should reject an unknown processor"
    );
}

} // namespace

int main() {
    test_rotation();
    test_red_formula_and_clamping();
    test_command_parser();

    if (failures == 0) {
        std::cout << "All processor tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
