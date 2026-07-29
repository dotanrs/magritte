#include "../common/test_support.h"
#include "magritte/parser.h"
#include "magritte/processors/rgb_formula.h"

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

const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"r", "r * 2 - g"}
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

const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"g", "b * 2"}
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

const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"b", "r - g"}
);

expect(processed.pixels[0].blue == 60, "blue formula should update blue");
expect(
    processed.pixels[0].red == 100 && processed.pixels[0].green == 40,
    "blue formula should not modify red or green"
);
}

void test_formula_coordinates_and_dimensions() {
const FileData processed = rgb_formula_processor().apply(
    blank_image(3, 2),
    {"r", "X + Y * 10 + W + H"}
);

expect(
    red_values(processed) ==
    std::vector<std::uint8_t>{5, 6, 7, 15, 16, 17},
    "formula coordinates should be zero-based and expose image dimensions"
);
}

void test_formula_normalized_and_polar_coordinates() {
const FileData horizontal = rgb_formula_processor().apply(
    blank_image(3, 1),
    {"r", "(U + 1) * 100"}
);
expect(
    red_values(horizontal) == std::vector<std::uint8_t>{0, 100, 200},
    "U should range from -1 to 1"
);

const FileData vertical = rgb_formula_processor().apply(
    blank_image(1, 3),
    {"r", "(V + 1) * 100"}
);
expect(
    red_values(vertical) == std::vector<std::uint8_t>{0, 100, 200},
    "V should range from -1 to 1"
);

const FileData single_pixel = rgb_formula_processor().apply(
    blank_image(1, 1),
    {"r", "100 + U + V"}
);
expect(
    single_pixel.pixels[0].red == 100,
    "normalized coordinates should be zero for one-pixel dimensions"
);

const FileData distance = rgb_formula_processor().apply(
    blank_image(3, 3),
    {"r", "D * 100"}
);
expect(
    red_values(distance) ==
    std::vector<std::uint8_t>{
        141, 100, 141,
        100, 0, 100,
        141, 100, 141,
    },
    "D should be pixel distance from the image center"
);

const FileData angle = rgb_formula_processor().apply(
    blank_image(3, 3),
    {"r", "round((A + PI) * 10)"}
);
expect(
    angle.pixels[5].red == 31 &&
    angle.pixels[7].red == 47 &&
    angle.pixels[3].red == 63 &&
    angle.pixels[1].red == 16,
    "A should be the clockwise image-space angle from the positive x-axis"
);

const FileData offset_polar = rgb_formula_processor().apply(
    blank_image(3, 3),
    {"rg", "(round((A + PI) * 10), D * 10)", "0", "0"}
);
expect(
    offset_polar.pixels[2].red == 31 &&
    offset_polar.pixels[6].red == 47 &&
    offset_polar.pixels[8].red == 39,
    "RGB offset should move the angle origin to normalized coordinates"
);
expect(
    offset_polar.pixels[0].green == 0 &&
    offset_polar.pixels[8].green == 28,
    "RGB offset should move the distance origin with the angle origin"
);
}

void test_formula_math_functions() {
const FileData trigonometry = rgb_formula_processor().apply(
    blank_image(1, 1),
    {"r", "10 * (sin(PI / 2) + cos(0) + tan(PI / 4))"}
);
expect(
    trigonometry.pixels[0].red == 30,
    "formula should evaluate trigonometric functions and PI"
);

const FileData arithmetic = rgb_formula_processor().apply(
    blank_image(1, 1),
    {
        "r",
        "abs(-2) + sqrt(9) + pow(2, 3) + mod(7, 4) + "
        "min(9, 5) + max(2, 6) + floor(2.9) + ceil(2.1) + "
        "round(2.5) + exp(0) + log(E) + atan2(0, -1)"
    }
);
expect(
    arithmetic.pixels[0].red == 40,
    "formula should evaluate arithmetic functions and E"
);

const FileData clamped = rgb_formula_processor().apply(
    blank_image(1, 1),
    {"r", "clamp(300, 200, 10)"}
);
expect(
    clamped.pixels[0].red == 200,
    "formula clamp should accept bounds in either order"
);
}

void test_formula_macros() {
FileData image{
    .width = 1,
    .height = 1,
    .pixels = {
        Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40},
    },
};
const MacroMap macros{
    {"macro_gain", "2"},
    {"macro_adjusted", "macro_gain + 1"},
};
const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"rgb", "(R * macro_adjusted, G + macro_gain, B)"},
    &macros
);
expect(
    processed.pixels[0].red == 30 &&
    processed.pixels[0].green == 22 &&
    processed.pixels[0].blue == 30 &&
    processed.pixels[0].alpha == 40,
    "formula macros should expand expressions and nested macro references"
);

expect(
    parse_processor_command(
        "r = R * macro_gain"
    ).has_value(),
    "processor validation should allow explicitly prefixed macro variables"
);

try {
    static_cast<void>(rgb_formula_processor().apply(
        blank_image(1, 1),
        {"r", "macro_missing"}
    ));
    expect(false, "formula application should reject an unknown macro");
} catch (const std::invalid_argument &error) {
    expect(
        std::string(error.what()).find("unknown macro") != std::string::npos,
        "formula application should describe an unknown macro"
    );
}

const MacroMap cyclic{
    {"macro_first", "macro_second"},
    {"macro_second", "macro_first"},
};
try {
    static_cast<void>(rgb_formula_processor().apply(
        blank_image(1, 1),
        {"r", "macro_first"},
        &cyclic
    ));
    expect(false, "formula application should reject cyclic macros");
} catch (const std::invalid_argument &error) {
    expect(
        std::string(error.what()).find("cyclic macro") != std::string::npos,
        "formula application should describe cyclic macros"
    );
}
}

void test_simultaneous_rgb_formula() {
FileData image{
    .width = 2,
    .height = 1,
    .pixels = {
        Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40},
        Pixel{.red = 100, .green = 150, .blue = 200, .alpha = 80},
    },
};

const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"(G, B, R)"}
);

expect(
    processed.pixels[0].red == 20 &&
    processed.pixels[0].green == 30 &&
    processed.pixels[0].blue == 10 &&
    processed.pixels[0].alpha == 40,
    "RGB formula should evaluate every channel from the original pixel"
);
expect(
    processed.pixels[1].red == 150 &&
    processed.pixels[1].green == 200 &&
    processed.pixels[1].blue == 100 &&
    processed.pixels[1].alpha == 80,
    "RGB formula should update all channels and preserve alpha"
);

const FileData coordinate_processed = rgb_formula_processor().apply(
    blank_image(2, 2),
    {
        "("
        "127 + 127 * sin(PI * U), "
        "127 + 127 * cos(PI * V), "
        "X * 100 + Y * 50"
        ")"
    }
);
expect(
    coordinate_processed.pixels[0].red == 127 &&
    coordinate_processed.pixels[0].green == 0 &&
    coordinate_processed.pixels[0].blue == 0 &&
    coordinate_processed.pixels[3].red == 127 &&
    coordinate_processed.pixels[3].green == 0 &&
    coordinate_processed.pixels[3].blue == 150,
    "RGB formula should support functions and coordinates in every component"
);
}

void test_rgb_formula_target_order() {
FileData image{
    .width = 2,
    .height = 1,
    .pixels = {
        Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40},
        Pixel{.red = 50, .green = 60, .blue = 70, .alpha = 80},
    },
};

const FileData processed = rgb_formula_processor().apply(
    std::move(image),
    {"br", "(R, B)"}
);

expect(
    processed.pixels[0].red == 30 &&
    processed.pixels[0].green == 20 &&
    processed.pixels[0].blue == 10 &&
    processed.pixels[0].alpha == 40,
    "a reordered RGB target should exchange selected channels"
);
expect(
    processed.pixels[1].red == 70 &&
    processed.pixels[1].green == 60 &&
    processed.pixels[1].blue == 50 &&
    processed.pixels[1].alpha == 80,
    "a reordered RGB target should preserve untargeted channels and alpha"
);

const FileData subset = rgb_formula_processor().apply(
    FileData{
        .width = 1,
        .height = 1,
        .pixels = {
            Pixel{.red = 10, .green = 20, .blue = 30, .alpha = 40},
        },
    },
    {"rg", "(G + B, R + B)"}
);
expect(
    subset.pixels[0].red == 50 &&
    subset.pixels[0].green == 40 &&
    subset.pixels[0].blue == 30 &&
    subset.pixels[0].alpha == 40,
    "a subset formula should read the original pixel simultaneously"
);
}
