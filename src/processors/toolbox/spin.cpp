#include "pixlie/processors/spin.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/utils/argument_parse.h"
#include "pixlie/processors/utils/image_sample.h"

namespace {
    struct SpinArguments {
        double center_x_percent;
        double center_y_percent;
        double angle_degrees;
        std::optional<double> radius_percent;
    };

    constexpr Pixel radius_hint{
        .red = 255,
        .green = 215,
        .blue = 0,
        .alpha = 255,
    };
    constexpr Pixel center_hint{
        .red = 255,
        .green = 0,
        .blue = 255,
        .alpha = 255,
    };
    constexpr Pixel angle_hint{
        .red = 0,
        .green = 220,
        .blue = 255,
        .alpha = 255,
    };

    double parse_number(const std::string &text, std::string_view name) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "spin " + std::string(name) + " must be a finite number"
            );
        }
        return value;
    }

    SpinArguments parse_spin_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() != 3 && arguments.size() != 4) {
            throw std::invalid_argument(
                "spin expects three or four numbers: x y angle [radius]"
            );
        }

        SpinArguments result{
            .center_x_percent = parse_number(arguments[0], "x"),
            .center_y_percent = parse_number(arguments[1], "y"),
            .angle_degrees = parse_number(arguments[2], "angle"),
            .radius_percent = arguments.size() == 4
                ? std::optional<double>(
                    parse_number(arguments[3], "radius")
                )
                : std::nullopt,
        };
        if (result.center_x_percent < 0.0 ||
            result.center_x_percent > 100.0 ||
            result.center_y_percent < 0.0 ||
            result.center_y_percent > 100.0) {
            throw std::invalid_argument(
                "spin x and y must be percentages from 0 to 100"
            );
        }
        if (result.radius_percent && *result.radius_percent <= 0.0) {
            throw std::invalid_argument(
                "spin radius must be greater than 0"
            );
        }
        return result;
    }

    double radians(double degrees) {
        return degrees * std::numbers::pi / 180.0;
    }

    void mark_pixel(
        FileData &data,
        long x,
        long y,
        const Pixel &hint
    ) {
        if (x < 0 || y < 0 ||
            static_cast<std::size_t>(x) >= data.width ||
            static_cast<std::size_t>(y) >= data.height) {
            return;
        }
        Pixel &pixel = data.pixels[
            static_cast<std::size_t>(y) * data.width +
            static_cast<std::size_t>(x)
        ];
        pixel.red = hint.red;
        pixel.green = hint.green;
        pixel.blue = hint.blue;
    }

    void mark_disk(
        FileData &data,
        double x,
        double y,
        long radius,
        const Pixel &hint
    ) {
        const long center_x = std::lround(x);
        const long center_y = std::lround(y);
        for (long offset_y = -radius; offset_y <= radius; ++offset_y) {
            for (long offset_x = -radius; offset_x <= radius; ++offset_x) {
                if (offset_x * offset_x + offset_y * offset_y <=
                    radius * radius) {
                    mark_pixel(
                        data,
                        center_x + offset_x,
                        center_y + offset_y,
                        hint
                    );
                }
            }
        }
    }

    FileData add_spin_debug_hints(
        FileData data,
        const SpinArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0) {
            return data;
        }

        const double shorter_dimension =
            static_cast<double>(std::min(data.width, data.height));
        const double center_x =
            static_cast<double>(data.width - 1) *
            arguments.center_x_percent / 100.0;
        const double center_y =
            static_cast<double>(data.height - 1) *
            arguments.center_y_percent / 100.0;
        const double guide_radius = arguments.radius_percent
            ? shorter_dimension * *arguments.radius_percent / 100.0
            : shorter_dimension / 2.0;
        const double line_half_width = std::max(
            0.75,
            shorter_dimension / 500.0
        );
        const long stroke_radius = std::max(
            1L,
            std::lround(line_half_width)
        );

        for (std::size_t y = 0; y < data.height; ++y) {
            for (std::size_t x = 0; x < data.width; ++x) {
                const double distance = std::hypot(
                    static_cast<double>(x) - center_x,
                    static_cast<double>(y) - center_y
                );
                if (std::abs(distance - guide_radius) <= line_half_width) {
                    mark_pixel(
                        data,
                        static_cast<long>(x),
                        static_cast<long>(y),
                        radius_hint
                    );
                }
            }
        }

        const double angle = radians(arguments.angle_degrees);
        for (double distance = 0.0;
             distance <= guide_radius;
             distance += 0.5) {
            mark_disk(
                data,
                center_x + distance * std::cos(angle),
                center_y + distance * std::sin(angle),
                stroke_radius,
                angle_hint
            );
        }

        const long cross_arm = std::max(
            2L,
            std::lround(shorter_dimension / 50.0)
        );
        for (long offset = -cross_arm; offset <= cross_arm; ++offset) {
            for (long width_offset = -stroke_radius;
                 width_offset <= stroke_radius;
                 ++width_offset) {
                mark_pixel(
                    data,
                    std::lround(center_x) + offset,
                    std::lround(center_y) + width_offset,
                    center_hint
                );
                mark_pixel(
                    data,
                    std::lround(center_x) + width_offset,
                    std::lround(center_y) + offset,
                    center_hint
                );
            }
        }
        return data;
    }

    FileData apply_spin(
        FileData data,
        const SpinArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 ||
            arguments.angle_degrees == 0.0) {
            return data;
        }

        const double center_x =
            static_cast<double>(data.width - 1) *
            arguments.center_x_percent / 100.0;
        const double center_y =
            static_cast<double>(data.height - 1) *
            arguments.center_y_percent / 100.0;
        const double radius = arguments.radius_percent
            ? static_cast<double>(std::min(data.width, data.height)) *
                *arguments.radius_percent / 100.0
            : 0.0;
        if (arguments.radius_percent && !std::isfinite(radius)) {
            throw std::invalid_argument("spin radius is too large");
        }
        const double angle_offset = radians(arguments.angle_degrees);
        FileData result{
            .width = data.width,
            .height = data.height,
            .pixels = std::vector<Pixel>(data.pixels.size()),
        };

        for (std::size_t y = 0; y < data.height; ++y) {
            for (std::size_t x = 0; x < data.width; ++x) {
                const double offset_x = static_cast<double>(x) - center_x;
                const double offset_y = static_cast<double>(y) - center_y;
                const double distance = std::hypot(offset_x, offset_y);
                const std::size_t index = y * data.width + x;
                if (arguments.radius_percent && distance >= radius) {
                    result.pixels[index] = data.pixels[index];
                    continue;
                }

                const double source_angle =
                    std::atan2(offset_y, offset_x) + angle_offset;
                const double source_x =
                    center_x + distance * std::cos(source_angle);
                const double source_y =
                    center_y + distance * std::sin(source_angle);
                result.pixels[index] =
                    sample_bilinear(data, source_x, source_y);
            }
        }
        return result;
    }

    class SpinProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "spin";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_keyword(command, "spin");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_spin_arguments(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return apply_spin(
                std::move(data),
                parse_spin_arguments(arguments)
            );
        }

        [[nodiscard]] FileData add_debug_hints(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return add_spin_debug_hints(
                std::move(data),
                parse_spin_arguments(arguments)
            );
        }
    };
} // namespace

const ImageProcessor &spin_processor() {
    static const SpinProcessor processor;
    return processor;
}
