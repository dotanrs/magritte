#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include "pixlie/parser.h"
#include "pixlie/processors/blue_formula.h"
#include "pixlie/processors/blur.h"
#include "pixlie/processors/color_swap.h"
#include "pixlie/processors/fisheye.h"
#include "pixlie/processors/green_formula.h"
#include "pixlie/processors/mirror.h"
#include "pixlie/processors/red_formula.h"
#include "pixlie/processors/rgb_formula.h"
#include "pixlie/processors/rotate.h"
#include "pixlie/processors/saturation_formula.h"
#include "pixlie/processors/warp_formula.h"

namespace {
    int failures = 0;

    void expect(bool condition, const std::string &message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    std::vector<std::uint8_t> red_values(const FileData &data) {
        std::vector<std::uint8_t> values;
        values.reserve(data.pixels.size());
        for (const Pixel &pixel: data.pixels) {
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

        const FileData across_x = mirror_processor().apply(image(), {"y"});
        expect(
            red_values(across_x) ==
            std::vector<std::uint8_t>{4, 5, 6, 1, 2, 3},
            "mirror y should exchange the top and bottom"
        );

        const FileData across_y = mirror_processor().apply(image(), {"x"});
        expect(
            red_values(across_y) ==
            std::vector<std::uint8_t>{3, 2, 1, 6, 5, 4},
            "mirror x should exchange the left and right"
        );
    }

    void test_blur() {
        FileData image{
            .width = 3,
            .height = 3,
            .pixels = std::vector<Pixel>(
                9,
                Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 40}
            ),
        };
        image.pixels[4] =
            Pixel{.red = 90, .green = 180, .blue = 45, .alpha = 200};

        const FileData blurred = blur_processor().apply(
            std::move(image),
            {"1"}
        );
        expect(
            red_values(blurred) ==
            std::vector<std::uint8_t>{23, 15, 23, 15, 10, 15, 23, 15, 23},
            "blur should average the available square neighborhood"
        );
        expect(
            blurred.pixels[0].green == 45 &&
            blurred.pixels[4].green == 20 &&
            blurred.pixels[0].blue == 11 &&
            blurred.pixels[4].blue == 5,
            "blur should process every RGB channel"
        );
        expect(
            blurred.pixels[0].alpha == 40 &&
            blurred.pixels[4].alpha == 200,
            "blur should preserve each pixel's alpha"
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

    FileData blank_image(std::size_t width, std::size_t height) {
        return FileData{
            .width = width,
            .height = height,
            .pixels = std::vector<Pixel>(
                width * height,
                Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
            ),
        };
    }

    void test_formula_coordinates_and_dimensions() {
        const FileData processed = red_formula_processor().apply(
            blank_image(3, 2),
            {"X + Y * 10 + W + H"}
        );

        expect(
            red_values(processed) ==
            std::vector<std::uint8_t>{5, 6, 7, 15, 16, 17},
            "formula coordinates should be zero-based and expose image dimensions"
        );
    }

    void test_formula_normalized_and_polar_coordinates() {
        const FileData horizontal = red_formula_processor().apply(
            blank_image(3, 1),
            {"(U + 1) * 100"}
        );
        expect(
            red_values(horizontal) == std::vector<std::uint8_t>{0, 100, 200},
            "U should range from -1 to 1"
        );

        const FileData vertical = red_formula_processor().apply(
            blank_image(1, 3),
            {"(V + 1) * 100"}
        );
        expect(
            red_values(vertical) == std::vector<std::uint8_t>{0, 100, 200},
            "V should range from -1 to 1"
        );

        const FileData single_pixel = red_formula_processor().apply(
            blank_image(1, 1),
            {"100 + U + V"}
        );
        expect(
            single_pixel.pixels[0].red == 100,
            "normalized coordinates should be zero for one-pixel dimensions"
        );

        const FileData distance = red_formula_processor().apply(
            blank_image(3, 3),
            {"D * 100"}
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

        const FileData angle = red_formula_processor().apply(
            blank_image(3, 3),
            {"round((A + PI) * 10)"}
        );
        expect(
            angle.pixels[5].red == 31 &&
            angle.pixels[7].red == 47 &&
            angle.pixels[3].red == 63 &&
            angle.pixels[1].red == 16,
            "A should be the clockwise image-space angle from the positive x-axis"
        );
    }

    void test_formula_math_functions() {
        const FileData trigonometry = red_formula_processor().apply(
            blank_image(1, 1),
            {"10 * (sin(PI / 2) + cos(0) + tan(PI / 4))"}
        );
        expect(
            trigonometry.pixels[0].red == 30,
            "formula should evaluate trigonometric functions and PI"
        );

        const FileData arithmetic = red_formula_processor().apply(
            blank_image(1, 1),
            {
                "abs(-2) + sqrt(9) + pow(2, 3) + mod(7, 4) + "
                "min(9, 5) + max(2, 6) + floor(2.9) + ceil(2.1) + "
                "round(2.5) + exp(0) + log(E) + atan2(0, -1)"
            }
        );
        expect(
            arithmetic.pixels[0].red == 40,
            "formula should evaluate arithmetic functions and E"
        );

        const FileData clamped = red_formula_processor().apply(
            blank_image(1, 1),
            {"clamp(300, 200, 10)"}
        );
        expect(
            clamped.pixels[0].red == 200,
            "formula clamp should accept bounds in either order"
        );
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

    void test_warp_formula() {
        const auto image = [] {
            return FileData{
                .width = 2,
                .height = 2,
                .pixels = {
                    Pixel{.red = 0, .green = 10, .blue = 20, .alpha = 0},
                    Pixel{.red = 100, .green = 30, .blue = 40, .alpha = 40},
                    Pixel{.red = 200, .green = 50, .blue = 60, .alpha = 80},
                    Pixel{.red = 240, .green = 70, .blue = 80, .alpha = 120},
                },
            };
        };

        const FileData identity = warp_formula_processor().apply(
            image(),
            {"(X, Y)"}
        );
        expect(
            identity.width == 2 &&
            identity.height == 2 &&
            identity.pixels[0].red == 0 &&
            identity.pixels[0].alpha == 0 &&
            identity.pixels[3].red == 240 &&
            identity.pixels[3].alpha == 120,
            "identity warp should preserve dimensions and pixels"
        );

        const FileData interpolated = warp_formula_processor().apply(
            image(),
            {"(0.5, 0.5)"}
        );
        expect(
            interpolated.pixels[0].red == 135 &&
            interpolated.pixels[0].green == 40 &&
            interpolated.pixels[0].blue == 50 &&
            interpolated.pixels[0].alpha == 60 &&
            interpolated.pixels[3].red == 135 &&
            interpolated.pixels[3].alpha == 60,
            "warp should bilinearly interpolate RGB and alpha"
        );

        const FileData clamped = warp_formula_processor().apply(
            image(),
            {"(-100, 100)"}
        );
        expect(
            clamped.pixels[0].red == 200 &&
            clamped.pixels[0].green == 50 &&
            clamped.pixels[0].blue == 60 &&
            clamped.pixels[0].alpha == 80 &&
            clamped.pixels[3].red == 200,
            "warp should clamp source coordinates to the image edges"
        );
    }

    void test_fisheye() {
        const auto image = [] {
            FileData result{
                .width = 5,
                .height = 5,
                .pixels = std::vector<Pixel>(
                    25,
                    Pixel{.red = 0, .green = 0, .blue = 0, .alpha = 255}
                ),
            };
            for (std::size_t y = 0; y < result.height; ++y) {
                for (std::size_t x = 0; x < result.width; ++x) {
                    Pixel &pixel = result.pixels[y * result.width + x];
                    pixel.red = static_cast<std::uint8_t>(x * 60);
                    pixel.green = static_cast<std::uint8_t>(y * 60);
                }
            }
            return result;
        };
        const auto middle_red = [](const FileData &data) {
            std::vector<std::uint8_t> values;
            for (std::size_t x = 0; x < data.width; ++x) {
                values.push_back(data.pixels[2 * data.width + x].red);
            }
            return values;
        };

        const FileData enlarged = fisheye_processor().apply(
            image(),
            {"50", "50", "1", "40"}
        );
        expect(
            middle_red(enlarged) ==
            std::vector<std::uint8_t>{0, 80, 120, 160, 240},
            "positive fisheye amount should enlarge around its center"
        );
        expect(
            enlarged.pixels[4].red == 240 &&
            enlarged.pixels[4].green == 0,
            "fisheye should preserve pixels outside its radius"
        );

        const FileData shrunk = fisheye_processor().apply(
            image(),
            {"50", "50", "-0.5", "40"}
        );
        expect(
            middle_red(shrunk) ==
            std::vector<std::uint8_t>{0, 40, 120, 200, 240},
            "negative fisheye amount should shrink around its center"
        );

        const FileData offset = fisheye_processor().apply(
            image(),
            {"25", "50", "1", "60"}
        );
        expect(
            middle_red(offset) ==
            std::vector<std::uint8_t>{24, 60, 96, 150, 240},
            "fisheye should use the supplied x and y as its center"
        );

        const FileData identity = fisheye_processor().apply(
            image(),
            {"50", "50", "0"}
        );
        expect(
            red_values(identity) == red_values(image()),
            "zero fisheye amount should preserve the image"
        );

        const FileData default_radius = fisheye_processor().apply(
            image(),
            {"50", "50", "1"}
        );
        const FileData explicit_default_radius = fisheye_processor().apply(
            image(),
            {"50", "50", "1", "100"}
        );
        expect(
            red_values(default_radius) == red_values(explicit_default_radius),
            "omitted fisheye radius should default to 100 percent"
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

        FileData coordinate_image{
            .width = 3,
            .height = 1,
            .pixels = std::vector<Pixel>(
                3,
                Pixel{.red = 191, .green = 64, .blue = 64, .alpha = 255}
            ),
        };
        const FileData coordinate_processed =
                saturation_formula_processor().apply(
                    std::move(coordinate_image),
                    {"(U + 1) * 127.5"}
                );
        expect(
            coordinate_processed.pixels[0].red == 128 &&
            coordinate_processed.pixels[0].green == 128 &&
            coordinate_processed.pixels[0].blue == 128 &&
            coordinate_processed.pixels[1].red == 191 &&
            coordinate_processed.pixels[1].green == 64 &&
            coordinate_processed.pixels[2].red == 255 &&
            coordinate_processed.pixels[2].green == 0,
            "saturation formulas should support coordinate variables"
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

    void test_processor_argument_parsing() {
        const auto rotate_arguments =
            rotate_processor().parse_arguments("rotate -1");
        expect(
            rotate_arguments ==
            std::optional<std::vector<std::string>>{{"-1"}},
            "rotate processor should parse its own arguments"
        );
        expect(
            !rotate_processor().parse_arguments("mirror x").has_value(),
            "rotate processor should decline another processor's command"
        );

        const auto rgb_arguments =
            rgb_formula_processor().parse_arguments("rgb = (G, B, R)");
        expect(
            rgb_arguments ==
            std::optional<std::vector<std::string>>{{"(G, B, R)"}},
            "RGB processor should parse its assignment"
        );
        expect(
            !rgb_formula_processor().parse_arguments("r = G").has_value(),
            "RGB processor should require its own assignment keyword"
        );

        const auto warp_arguments =
            warp_formula_processor().parse_arguments(
                "warp = (X + sin(Y), Y)"
            );
        expect(
            warp_arguments ==
            std::optional<std::vector<std::string>>{
                {"(X + sin(Y), Y)"}
            },
            "warp processor should parse its coordinate assignment"
        );
        expect(
            !warp_formula_processor().parse_arguments(
                "rgb = (R, G, B)"
            ).has_value(),
            "warp processor should require its own assignment keyword"
        );

        const auto fisheye_arguments =
            fisheye_processor().parse_arguments("fisheye 50 50 1 25");
        expect(
            fisheye_arguments ==
            std::optional<std::vector<std::string>>{
                {"50", "50", "1", "25"}
            },
            "fisheye processor should parse its center, amount, and radius"
        );
        expect(
            !fisheye_processor().parse_arguments("blur 3").has_value(),
            "fisheye processor should decline another processor's command"
        );

        const auto swap_arguments =
            color_swap_processor().parse_arguments("r <-> b");
        expect(
            swap_arguments ==
            std::optional<std::vector<std::string>>{{"r", "b"}},
            "color swap processor should parse arguments around its operator"
        );
    }

    void test_command_parser() {
        std::string error_message;

        expect(
            parse_processor_command("rotate -1").has_value(),
            "parser should accept a valid rotate command"
        );
        expect(
            parse_processor_command("mirror x").has_value(),
            "parser should accept a valid mirror command"
        );
        expect(
            parse_processor_command("blur 3").has_value(),
            "parser should accept a valid blur command"
        );
        expect(
            parse_processor_command("fisheye 50 50 -0.5").has_value(),
            "parser should accept a fisheye command with default radius"
        );
        expect(
            parse_processor_command("fisheye 50 50 -0.5 25").has_value(),
            "parser should accept a fisheye command with explicit radius"
        );
        expect(
            parse_processor_command("r = (R + G) / 2").has_value(),
            "parser should accept a valid red formula"
        );
        expect(
            parse_processor_command(
                "r = 127 + 127 * sin(X / 12 + A)"
            ).has_value(),
            "parser should accept coordinate-aware mathematical formulas"
        );
        expect(
            parse_processor_command(
                "rgb = (G, B, R)"
            ).has_value(),
            "parser should accept a simultaneous RGB formula"
        );
        expect(
            parse_processor_command(
                "rgb = (max(R, G), min(G, B), clamp(B, 0, 255))"
            ).has_value(),
            "RGB tuple separators should coexist with function arguments"
        );
        expect(
            parse_processor_command(
                "warp = (X + sin(Y), clamp(Y, 0, H - 1))"
            ).has_value(),
            "parser should accept a mathematical warp formula"
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
            !parse_processor_command("mirror z", &error_message).has_value(),
            "parser should reject an invalid mirror axis"
        );
        expect(
            error_message == "mirror expects exactly one axis: x or y",
            "parser should describe an invalid mirror axis"
        );
        error_message.clear();
        expect(
            !parse_processor_command("blur -1", &error_message).has_value(),
            "parser should reject a negative blur radius"
        );
        expect(
            error_message == "blur radius must be a nonnegative integer",
            "parser should describe an invalid blur radius"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "fisheye 100 75 -1",
                &error_message
            ).has_value(),
            "parser should reject a singular fisheye amount"
        );
        expect(
            error_message == "fisheye amount must be greater than -1",
            "parser should describe an invalid fisheye amount"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "fisheye 101 50 1",
                &error_message
            ).has_value(),
            "parser should reject an out-of-range fisheye percentage"
        );
        expect(
            error_message ==
            "fisheye x and y must be percentages from 0 to 100",
            "parser should describe invalid fisheye percentages"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "fisheye 100 75",
                &error_message
            ).has_value(),
            "parser should require all three fisheye arguments"
        );
        expect(
            error_message ==
            "fisheye expects three or four numbers: x y amount [radius]",
            "parser should describe missing fisheye arguments"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "fisheye 50 50 1 0",
                &error_message
            ).has_value(),
            "parser should reject a nonpositive fisheye radius"
        );
        expect(
            error_message == "fisheye radius must be greater than 0",
            "parser should describe an invalid fisheye radius"
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
        error_message.clear();
        expect(
            !parse_processor_command(
                "r = mystery(X)",
                &error_message
            ).has_value(),
            "parser should reject unknown mathematical functions"
        );
        expect(
            error_message.find("unknown function") != std::string::npos,
            "parser should describe an unknown mathematical function"
        );
        error_message.clear();
        expect(
            !parse_processor_command("r = pow(2)", &error_message).has_value(),
            "parser should reject a function with the wrong arity"
        );
        expect(
            error_message.find("expects 2 arguments") != std::string::npos,
            "parser should describe incorrect function arity"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "rgb = R, G, B",
                &error_message
            ).has_value(),
            "parser should require parentheses around an RGB tuple"
        );
        expect(
            error_message.find("parenthesized tuple") != std::string::npos,
            "parser should describe a missing RGB tuple"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "rgb = (R, G)",
                &error_message
            ).has_value(),
            "parser should require exactly three RGB expressions"
        );
        expect(
            !error_message.empty(),
            "parser should describe an incomplete RGB tuple"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "rgb = (R, G, B, 0)",
                &error_message
            ).has_value(),
            "parser should reject more than three RGB expressions"
        );
        expect(
            error_message.find("expected ')'") != std::string::npos,
            "parser should describe an oversized RGB tuple"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "warp = X, Y",
                &error_message
            ).has_value(),
            "parser should require parentheses around warp coordinates"
        );
        expect(
            error_message.find("parenthesized coordinate pair") !=
            std::string::npos,
            "parser should describe a missing warp coordinate pair"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "warp = (X)",
                &error_message
            ).has_value(),
            "parser should require two warp expressions"
        );
        expect(
            !error_message.empty(),
            "parser should describe an incomplete warp formula"
        );
        error_message.clear();
        expect(
            !parse_processor_command(
                "warp = (X, Y, 0)",
                &error_message
            ).has_value(),
            "parser should reject more than two warp expressions"
        );
        expect(
            error_message.find("expected ')'") != std::string::npos,
            "parser should describe an oversized warp coordinate pair"
        );
    }
} // namespace

int main() {
    test_rotation();
    test_mirror();
    test_blur();
    test_red_formula_and_clamping();
    test_green_formula();
    test_blue_formula();
    test_formula_coordinates_and_dimensions();
    test_formula_normalized_and_polar_coordinates();
    test_formula_math_functions();
    test_simultaneous_rgb_formula();
    test_warp_formula();
    test_fisheye();
    test_saturation_formula();
    test_color_swap();
    test_processor_argument_parsing();
    test_command_parser();

    if (failures == 0) {
        std::cout << "All processor tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
