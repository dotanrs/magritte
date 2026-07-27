#include "pixlie/processors/lighting.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/utils/argument_parse.h"

namespace {
    struct Light {
        double source_angle_degrees;
        Pixel color;
        std::optional<int> luminance_threshold;
        double strength;
        double softness_percent;
        double atmosphere;
        double shadow;
    };

    struct LightingArguments {
        std::vector<Light> lights;
    };

    struct Vector {
        double x;
        double y;
    };

    using LightMap = std::vector<float>;

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

    double unit_value(
        const std::string &text,
        std::string_view name
    ) {
        const double value = parse_number(text, name);
        if (value < 0.0 || value > 1.0) {
            throw std::invalid_argument(
                "lighting " + std::string(name) + " must be from 0 to 1"
            );
        }
        return value;
    }

    double softness_value(const std::string &text) {
        const double value = parse_number(text, "softness");
        if (value < 0.0 || value > 50.0) {
            throw std::invalid_argument(
                "lighting softness must be from 0 to 50 percent"
            );
        }
        return value;
    }

    std::optional<int> parse_threshold_or_auto(const std::string &text) {
        if (text == "auto") {
            return std::nullopt;
        }
        return parse_threshold(text);
    }

    Light make_light(
        double angle,
        std::string_view color,
        double strength,
        double softness,
        double atmosphere,
        double shadow
    ) {
        return Light{
            .source_angle_degrees = angle,
            .color = parse_color(color),
            .luminance_threshold = std::nullopt,
            .strength = strength,
            .softness_percent = softness,
            .atmosphere = atmosphere,
            .shadow = shadow,
        };
    }

    std::optional<LightingArguments> preset(
        std::string_view name,
        double amount
    ) {
        if (name == "golden-hour") {
            return LightingArguments{.lights = {
                make_light(315.0, "#FFB36B", 0.78 * amount, 10.0, 0.08, 0.20),
            }};
        }
        if (name == "moonlight") {
            return LightingArguments{.lights = {
                make_light(225.0, "#759BFF", 0.68 * amount, 14.0, 0.13, 0.26),
            }};
        }
        if (name == "studio") {
            return LightingArguments{.lights = {
                make_light(225.0, "#FFD7AE", 0.62 * amount, 16.0, 0.03, 0.16),
                make_light(315.0, "#82B8FF", 0.34 * amount, 8.0, 0.02, 0.05),
            }};
        }
        if (name == "synthwave") {
            return LightingArguments{.lights = {
                make_light(180.0, "#FF3CAC", 0.58 * amount, 12.0, 0.12, 0.13),
                make_light(0.0, "#35D6FF", 0.55 * amount, 12.0, 0.10, 0.12),
            }};
        }
        return std::nullopt;
    }

    LightingArguments parse_lighting_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() == 1 || arguments.size() == 2) {
            const double amount = arguments.size() == 2
                ? unit_value(arguments[1], "preset strength")
                : 1.0;
            if (auto result = preset(arguments[0], amount)) {
                return std::move(*result);
            }
            throw std::invalid_argument(
                "unknown lighting preset; use golden-hour, moonlight, "
                "studio, or synthwave"
            );
        }

        if (arguments.size() < 3 || arguments.size() > 6) {
            throw std::invalid_argument(
                "lighting expects a preset [strength], or: angle #RRGGBB "
                "threshold|auto [strength [softness [atmosphere]]]"
            );
        }

        return LightingArguments{.lights = {
            Light{
                .source_angle_degrees = parse_number(arguments[0], "angle"),
                .color = parse_color(arguments[1]),
                .luminance_threshold =
                    parse_threshold_or_auto(arguments[2]),
                .strength = arguments.size() >= 4
                    ? unit_value(arguments[3], "strength")
                    : 0.78,
                .softness_percent = arguments.size() >= 5
                    ? softness_value(arguments[4])
                    : 8.0,
                .atmosphere = arguments.size() >= 6
                    ? unit_value(arguments[5], "atmosphere")
                    : 0.06,
                .shadow = 0.16,
            },
        }};
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

    std::uint8_t gel_light_channel(
        std::uint8_t original,
        std::uint8_t light,
        double amount
    ) {
        constexpr double tint = 0.35;
        const double original_value = static_cast<double>(original);
        const double light_value = static_cast<double>(light);
        const double screened =
            original_value +
            (255.0 - original_value) * light_value / 255.0;
        const double target =
            screened * (1.0 - tint) + light_value * tint;
        const double result =
            original_value + (target - original_value) * amount;
        return static_cast<std::uint8_t>(std::lround(result));
    }

    double luminance(const Pixel &pixel) {
        return (
            2126.0 * static_cast<double>(pixel.red) +
            7152.0 * static_cast<double>(pixel.green) +
            722.0 * static_cast<double>(pixel.blue)
        ) / 10000.0;
    }

    LightMap image_luminance(const FileData &data) {
        LightMap result;
        result.reserve(data.pixels.size());
        for (const Pixel &pixel: data.pixels) {
            result.push_back(static_cast<float>(luminance(pixel)));
        }
        return result;
    }

    int automatic_threshold(const LightMap &values) {
        if (values.empty()) {
            return 128;
        }

        // Otsu's method finds the strongest luminance split without requiring
        // the caller to know whether the source is a dark scene or a bright photo.
        std::array<std::size_t, 256> histogram{};
        int minimum = 255;
        int maximum = 0;
        for (const double value: values) {
            const int bin = std::clamp(
                static_cast<int>(std::lround(value)),
                0,
                255
            );
            ++histogram[static_cast<std::size_t>(bin)];
            minimum = std::min(minimum, bin);
            maximum = std::max(maximum, bin);
        }
        if (minimum == maximum) {
            return minimum;
        }

        double total_sum = 0.0;
        for (std::size_t value = 0; value < histogram.size(); ++value) {
            total_sum += static_cast<double>(value) *
                         static_cast<double>(histogram[value]);
        }

        const std::size_t total = values.size();
        std::size_t background_count = 0;
        double background_sum = 0.0;
        double best_variance = -1.0;
        int best_threshold = minimum;
        for (int threshold = minimum; threshold < maximum; ++threshold) {
            const std::size_t count =
                histogram[static_cast<std::size_t>(threshold)];
            background_count += count;
            background_sum +=
                static_cast<double>(threshold) * static_cast<double>(count);
            if (background_count == 0) {
                continue;
            }
            const std::size_t foreground_count = total - background_count;
            if (foreground_count == 0) {
                break;
            }

            const double background_mean =
                background_sum / static_cast<double>(background_count);
            const double foreground_mean =
                (total_sum - background_sum) /
                static_cast<double>(foreground_count);
            const double difference = background_mean - foreground_mean;
            const double variance =
                static_cast<double>(background_count) *
                static_cast<double>(foreground_count) *
                difference * difference;
            if (variance > best_variance) {
                best_variance = variance;
                best_threshold = threshold;
            }
        }
        return best_threshold;
    }

    LightMap box_blur(
        const LightMap &input,
        std::size_t width,
        std::size_t height,
        std::size_t radius
    ) {
        if (radius == 0 || width == 0 || height == 0) {
            return input;
        }

        LightMap horizontal(input.size());
        for (std::size_t y = 0; y < height; ++y) {
            const std::size_t row = y * width;
            double sum = 0.0;
            std::size_t right = std::min(width - 1, radius);
            for (std::size_t x = 0; x <= right; ++x) {
                sum += input[row + x];
            }
            for (std::size_t x = 0; x < width; ++x) {
                const std::size_t left = x > radius ? x - radius : 0;
                right = std::min(width - 1, x + radius);
                horizontal[row + x] =
                    sum / static_cast<double>(right - left + 1);
                if (x >= radius) {
                    sum -= input[row + x - radius];
                }
                if (x + radius + 1 < width) {
                    sum += input[row + x + radius + 1];
                }
            }
        }

        LightMap result(input.size());
        for (std::size_t x = 0; x < width; ++x) {
            double sum = 0.0;
            std::size_t bottom = std::min(height - 1, radius);
            for (std::size_t y = 0; y <= bottom; ++y) {
                sum += horizontal[y * width + x];
            }
            for (std::size_t y = 0; y < height; ++y) {
                const std::size_t top = y > radius ? y - radius : 0;
                bottom = std::min(height - 1, y + radius);
                result[y * width + x] =
                    sum / static_cast<double>(bottom - top + 1);
                if (y >= radius) {
                    sum -= horizontal[(y - radius) * width + x];
                }
                if (y + radius + 1 < height) {
                    sum += horizontal[(y + radius + 1) * width + x];
                }
            }
        }
        return result;
    }

    LightMap soften_visibility(
        const std::vector<std::uint8_t> &hard_visibility,
        std::size_t width,
        std::size_t height,
        double softness_percent
    ) {
        LightMap result;
        result.reserve(hard_visibility.size());
        for (const std::uint8_t value: hard_visibility) {
            result.push_back(value == 0 ? 0.0 : 1.0);
        }

        const std::size_t smaller_dimension = std::min(width, height);
        const std::size_t total_radius = static_cast<std::size_t>(std::lround(
            static_cast<double>(smaller_dimension) *
            softness_percent / 100.0
        ));
        if (total_radius == 0) {
            return result;
        }

        const std::size_t pass_radius = std::max<std::size_t>(
            1,
            (total_radius + 1) / 2
        );
        result = box_blur(result, width, height, pass_radius);
        return box_blur(result, width, height, pass_radius);
    }

    LightMap visibility_map(
        const FileData &data,
        const Light &light,
        int threshold
    ) {
        const Vector direction =
            light_travel_direction(light.source_angle_degrees);
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
        std::vector<std::uint8_t> hard_visibility(data.pixels.size(), 0);

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
                threshold,
                hard_visibility
            );
        }
        return soften_visibility(
            hard_visibility,
            data.width,
            data.height,
            light.softness_percent
        );
    }

    LightMap source_proximity(
        std::size_t width,
        std::size_t height,
        double source_angle_degrees
    ) {
        const double radians =
            std::fmod(source_angle_degrees, 360.0) *
            std::numbers::pi / 180.0;
        const Vector toward_source{
            .x = std::cos(radians),
            .y = std::sin(radians),
        };
        const double pixel_width = static_cast<double>(width);
        const double pixel_height = static_cast<double>(height);
        const double projections[] = {
            0.0,
            toward_source.x * pixel_width,
            toward_source.y * pixel_height,
            toward_source.x * pixel_width +
                toward_source.y * pixel_height,
        };
        const auto [minimum, maximum] =
            std::minmax_element(std::begin(projections), std::end(projections));
        const double span = *maximum - *minimum;

        LightMap result(width * height, 1.0F);
        if (span < component_epsilon) {
            return result;
        }
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const double projection =
                    toward_source.x * (static_cast<double>(x) + 0.5) +
                    toward_source.y * (static_cast<double>(y) + 0.5);
                result[y * width + x] = std::clamp(
                    (projection - *minimum) / span,
                    0.0,
                    1.0
                );
            }
        }
        return result;
    }

    double smoothstep(double lower, double upper, double value) {
        if (upper <= lower) {
            return value >= upper ? 1.0 : 0.0;
        }
        const double amount =
            std::clamp((value - lower) / (upper - lower), 0.0, 1.0);
        return amount * amount * (3.0 - 2.0 * amount);
    }

    void apply_light(
        FileData &data,
        const FileData &source,
        const LightMap &luminances,
        const Light &light
    ) {
        if (light.strength == 0.0) {
            return;
        }

        const int threshold = light.luminance_threshold.value_or(
            automatic_threshold(luminances)
        );
        const LightMap visibility =
            visibility_map(source, light, threshold);
        const LightMap proximity = source_proximity(
            data.width,
            data.height,
            light.source_angle_degrees
        );
        const double material_lower =
            std::max(0.0, static_cast<double>(threshold) * 0.65);
        const double material_upper =
            std::min(255.0, static_cast<double>(threshold) + 48.0);

        for (std::size_t index = 0; index < data.pixels.size(); ++index) {
            const double material = smoothstep(
                material_lower,
                material_upper,
                luminances[index]
            );
            const double directional_rolloff =
                0.35 + 0.65 * proximity[index];
            const double soft_occlusion =
                0.68 + 0.32 * visibility[index];
            // Scene structure receives the gel light; pixels below the chosen
            // luminance split receive only the much smaller atmospheric fill.
            const double exposure = std::clamp(
                light.strength * directional_rolloff * (
                    light.atmosphere * (1.0 - material) +
                    material * soft_occlusion
                ),
                0.0,
                1.0
            );
            const double shadow = std::clamp(
                light.shadow * light.strength * material *
                (1.0 - proximity[index]) *
                (0.65 + 0.35 * (1.0 - visibility[index])),
                0.0,
                1.0
            );

            Pixel &pixel = data.pixels[index];
            pixel.red = static_cast<std::uint8_t>(std::lround(
                static_cast<double>(pixel.red) * (1.0 - shadow)
            ));
            pixel.green = static_cast<std::uint8_t>(std::lround(
                static_cast<double>(pixel.green) * (1.0 - shadow)
            ));
            pixel.blue = static_cast<std::uint8_t>(std::lround(
                static_cast<double>(pixel.blue) * (1.0 - shadow)
            ));
            pixel.red = gel_light_channel(
                pixel.red,
                light.color.red,
                exposure
            );
            pixel.green = gel_light_channel(
                pixel.green,
                light.color.green,
                exposure
            );
            pixel.blue = gel_light_channel(
                pixel.blue,
                light.color.blue,
                exposure
            );
        }
    }

    FileData apply_lighting(
        FileData data,
        const LightingArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0) {
            return data;
        }

        const FileData source = data;
        const LightMap luminances = image_luminance(source);
        for (const Light &light: arguments.lights) {
            apply_light(data, source, luminances, light);
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
