#include "pixlie/processors/lighting.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/utils/argument_parse.h"

namespace {
    struct LightingArguments {
        double source_angle_degrees;
        Pixel color;
        int luminance_threshold;
        double strength;
    };

    struct Vector {
        double x;
        double y;
    };

    constexpr double component_epsilon = 1e-12;
    constexpr double entry_nudge = 1e-7;
    constexpr double ray_spacing = 0.5;

    double parse_number(const std::string &text, std::string_view name) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "lighting " + std::string(name) + " must be a finite number"
            );
        }
        return value;
    }

    std::uint8_t parse_hex_pair(
        std::string_view color,
        std::size_t offset
    ) {
        unsigned int value = 0;
        const char *begin = color.data() + offset;
        const auto [end, error] =
            std::from_chars(begin, begin + 2, value, 16);
        if (error != std::errc{} || end != begin + 2) {
            throw std::invalid_argument(
                "lighting color must use the form #RRGGBB"
            );
        }
        return static_cast<std::uint8_t>(value);
    }

    Pixel parse_color(std::string_view color) {
        if (color.size() != 7 || color.front() != '#') {
            throw std::invalid_argument(
                "lighting color must use the form #RRGGBB"
            );
        }
        return Pixel{
            .red = parse_hex_pair(color, 1),
            .green = parse_hex_pair(color, 3),
            .blue = parse_hex_pair(color, 5),
            .alpha = 255,
        };
    }

    int parse_threshold(const std::string &text) {
        int threshold = 0;
        const auto [end, error] = std::from_chars(
            text.data(),
            text.data() + text.size(),
            threshold
        );
        if (error != std::errc{} || end != text.data() + text.size() ||
            threshold < 0 || threshold > 255) {
            throw std::invalid_argument(
                "lighting threshold must be an integer from 0 to 255"
            );
        }
        return threshold;
    }

    LightingArguments parse_lighting_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() != 3 && arguments.size() != 4) {
            throw std::invalid_argument(
                "lighting expects: angle #RRGGBB threshold [strength]"
            );
        }

        const double strength = arguments.size() == 4
            ? parse_number(arguments[3], "strength")
            : 1.0;
        if (strength < 0.0 || strength > 1.0) {
            throw std::invalid_argument(
                "lighting strength must be from 0 to 1"
            );
        }

        return LightingArguments{
            .source_angle_degrees = parse_number(arguments[0], "angle"),
            .color = parse_color(arguments[1]),
            .luminance_threshold = parse_threshold(arguments[2]),
            .strength = strength,
        };
    }

    Vector light_travel_direction(double source_angle_degrees) {
        const double normalized_degrees =
            std::fmod(source_angle_degrees, 360.0);
        const double radians =
            normalized_degrees * std::numbers::pi / 180.0;
        Vector direction{
            .x = -std::cos(radians),
            .y = -std::sin(radians),
        };
        if (std::abs(direction.x) < component_epsilon) {
            direction.x = 0.0;
        }
        if (std::abs(direction.y) < component_epsilon) {
            direction.y = 0.0;
        }
        return direction;
    }

    bool reaches_luminance_threshold(const Pixel &pixel, int threshold) {
        constexpr std::uint32_t red_weight = 2126;
        constexpr std::uint32_t green_weight = 7152;
        constexpr std::uint32_t blue_weight = 722;
        constexpr std::uint32_t weight_scale = 10000;

        const std::uint32_t luminance =
            red_weight * pixel.red +
            green_weight * pixel.green +
            blue_weight * pixel.blue;
        return luminance >=
               static_cast<std::uint32_t>(threshold) * weight_scale;
    }

    bool intersect_axis(
        double origin,
        double direction,
        double maximum,
        double &entry,
        double &exit
    ) {
        if (std::abs(direction) < component_epsilon) {
            return origin >= 0.0 && origin <= maximum;
        }

        double first = -origin / direction;
        double last = (maximum - origin) / direction;
        if (first > last) {
            std::swap(first, last);
        }
        entry = std::max(entry, first);
        exit = std::min(exit, last);
        return entry <= exit;
    }

    bool ray_rectangle_interval(
        Vector perpendicular,
        Vector direction,
        double perpendicular_offset,
        double width,
        double height,
        double &entry,
        double &exit
    ) {
        entry = -std::numeric_limits<double>::infinity();
        exit = std::numeric_limits<double>::infinity();
        return intersect_axis(
                   perpendicular.x * perpendicular_offset,
                   direction.x,
                   width,
                   entry,
                   exit
               ) &&
               intersect_axis(
                   perpendicular.y * perpendicular_offset,
                   direction.y,
                   height,
                   entry,
                   exit
               ) &&
               entry < exit;
    }

    void trace_ray(
        const FileData &data,
        Vector perpendicular,
        Vector direction,
        double perpendicular_offset,
        int threshold,
        std::vector<std::uint8_t> &lit
    ) {
        const double width = static_cast<double>(data.width);
        const double height = static_cast<double>(data.height);
        double entry = 0.0;
        double exit = 0.0;
        if (!ray_rectangle_interval(
                perpendicular,
                direction,
                perpendicular_offset,
                width,
                height,
                entry,
                exit
            )) {
            return;
        }

        const double start = std::min(entry + entry_nudge, exit);
        double x = perpendicular.x * perpendicular_offset +
                   direction.x * start;
        double y = perpendicular.y * perpendicular_offset +
                   direction.y * start;
        x = std::clamp(x, 0.0, std::nextafter(width, 0.0));
        y = std::clamp(y, 0.0, std::nextafter(height, 0.0));

        long cell_x = static_cast<long>(std::floor(x));
        long cell_y = static_cast<long>(std::floor(y));
        const int step_x = direction.x > 0.0
            ? 1
            : direction.x < 0.0 ? -1 : 0;
        const int step_y = direction.y > 0.0
            ? 1
            : direction.y < 0.0 ? -1 : 0;
        const double delta_x = step_x == 0
            ? std::numeric_limits<double>::infinity()
            : 1.0 / std::abs(direction.x);
        const double delta_y = step_y == 0
            ? std::numeric_limits<double>::infinity()
            : 1.0 / std::abs(direction.y);
        double next_x = step_x > 0
            ? (static_cast<double>(cell_x + 1) - x) / direction.x
            : step_x < 0
                ? (x - static_cast<double>(cell_x)) / -direction.x
                : std::numeric_limits<double>::infinity();
        double next_y = step_y > 0
            ? (static_cast<double>(cell_y + 1) - y) / direction.y
            : step_y < 0
                ? (y - static_cast<double>(cell_y)) / -direction.y
                : std::numeric_limits<double>::infinity();

        while (cell_x >= 0 && cell_y >= 0 &&
               static_cast<std::size_t>(cell_x) < data.width &&
               static_cast<std::size_t>(cell_y) < data.height) {
            const std::size_t index =
                static_cast<std::size_t>(cell_y) * data.width +
                static_cast<std::size_t>(cell_x);
            lit[index] = 1;
            if (reaches_luminance_threshold(
                    data.pixels[index],
                    threshold
                )) {
                return;
            }

            if (next_x < next_y) {
                cell_x += step_x;
                next_x += delta_x;
            } else if (next_y < next_x) {
                cell_y += step_y;
                next_y += delta_y;
            } else {
                cell_x += step_x;
                cell_y += step_y;
                next_x += delta_x;
                next_y += delta_y;
            }
        }
    }

    std::uint8_t screen_channel(
        std::uint8_t original,
        std::uint8_t light,
        double strength
    ) {
        const double result =
            static_cast<double>(original) +
            (255.0 - static_cast<double>(original)) *
            static_cast<double>(light) * strength / 255.0;
        return static_cast<std::uint8_t>(std::lround(result));
    }

    FileData apply_lighting(
        FileData data,
        const LightingArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 ||
            arguments.strength == 0.0) {
            return data;
        }

        const Vector direction =
            light_travel_direction(arguments.source_angle_degrees);
        const Vector perpendicular{
            .x = -direction.y,
            .y = direction.x,
        };
        const double width = static_cast<double>(data.width);
        const double height = static_cast<double>(data.height);
        const double projections[] = {
            0.0,
            perpendicular.x * width,
            perpendicular.y * height,
            perpendicular.x * width + perpendicular.y * height,
        };
        const auto [minimum, maximum] =
            std::minmax_element(std::begin(projections), std::end(projections));
        const double span = *maximum - *minimum;
        const std::size_t ray_count = std::max<std::size_t>(
            1,
            static_cast<std::size_t>(std::ceil(span / ray_spacing))
        );
        std::vector<std::uint8_t> lit(data.pixels.size(), 0);

        for (std::size_t ray = 0; ray < ray_count; ++ray) {
            const double offset =
                *minimum +
                (static_cast<double>(ray) + 0.5) * span /
                static_cast<double>(ray_count);
            trace_ray(
                data,
                perpendicular,
                direction,
                offset,
                arguments.luminance_threshold,
                lit
            );
        }

        for (std::size_t index = 0; index < data.pixels.size(); ++index) {
            if (lit[index] == 0) {
                continue;
            }
            Pixel &pixel = data.pixels[index];
            pixel.red = screen_channel(
                pixel.red,
                arguments.color.red,
                arguments.strength
            );
            pixel.green = screen_channel(
                pixel.green,
                arguments.color.green,
                arguments.strength
            );
            pixel.blue = screen_channel(
                pixel.blue,
                arguments.color.blue,
                arguments.strength
            );
        }
        return data;
    }

    class LightingProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "lighting";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_keyword(command, "lighting");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_lighting_arguments(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return apply_lighting(
                std::move(data),
                parse_lighting_arguments(arguments)
            );
        }
    };
} // namespace

const ImageProcessor &lighting_processor() {
    static const LightingProcessor processor;
    return processor;
}
