// Processor: `fisheye <x> <y> <amount> [radius]`.
// Applies a radial lens distortion. `x` and `y` are center coordinates as
// percentages from 0 to 100; `amount` is greater than -1, with positive
// values magnifying, negative values shrinking, and 0 a no-op; optional
// `radius` is a positive percentage of the shorter image dimension and
// defaults to 100.

#include "pixlie/processors/fisheye.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/argument_parse.h"
#include "pixlie/processors/utils/image_sample.h"

namespace {
    struct FisheyeArguments {
        double center_x_percent;
        double center_y_percent;
        double amount;
        double radius_percent;
    };

    struct FisheyeGeometry {
        double center_x;
        double center_y;
        double radius;
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

    double parse_number(const std::string &text, std::string_view name) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "fisheye " + std::string(name) + " must be a finite number"
            );
        }
        return value;
    }

    FisheyeArguments parse_fisheye_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() != 3 && arguments.size() != 4) {
            throw std::invalid_argument(
                "fisheye expects three or four numbers: x y amount [radius]"
            );
        }

        FisheyeArguments result{
            .center_x_percent = parse_number(arguments[0], "x"),
            .center_y_percent = parse_number(arguments[1], "y"),
            .amount = parse_number(arguments[2], "amount"),
            .radius_percent = arguments.size() == 4
                ? parse_number(arguments[3], "radius")
                : 100.0,
        };
        if (result.center_x_percent < 0.0 ||
            result.center_x_percent > 100.0 ||
            result.center_y_percent < 0.0 ||
            result.center_y_percent > 100.0) {
            throw std::invalid_argument(
                "fisheye x and y must be percentages from 0 to 100"
            );
        }
        if (result.amount <= -1.0) {
            throw std::invalid_argument(
                "fisheye amount must be greater than -1"
            );
        }
        if (result.radius_percent <= 0.0) {
            throw std::invalid_argument(
                "fisheye radius must be greater than 0"
            );
        }
        return result;
    }

    FisheyeGeometry calculate_geometry(
        const FileData &data,
        const FisheyeArguments &arguments
    ) {
        const double last_x = static_cast<double>(data.width - 1);
        const double last_y = static_cast<double>(data.height - 1);
        const double radius =
                static_cast<double>(std::min(data.width, data.height)) *
                arguments.radius_percent / 100.0;
        if (!std::isfinite(radius)) {
            throw std::invalid_argument("fisheye radius is too large");
        }

        return FisheyeGeometry{
            .center_x = last_x * arguments.center_x_percent / 100.0,
            .center_y = last_y * arguments.center_y_percent / 100.0,
            .radius = radius,
        };
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

    FileData add_fisheye_debug_hints(
        FileData data,
        const FisheyeArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0) {
            return data;
        }

        const FisheyeGeometry geometry =
                calculate_geometry(data, arguments);
        const double line_half_width = std::max(
            0.75,
            static_cast<double>(std::min(data.width, data.height)) / 500.0
        );
        const long stroke_radius = std::max(
            1L,
            std::lround(line_half_width)
        );

        for (std::size_t y = 0; y < data.height; ++y) {
            for (std::size_t x = 0; x < data.width; ++x) {
                const double distance = std::hypot(
                    static_cast<double>(x) - geometry.center_x,
                    static_cast<double>(y) - geometry.center_y
                );
                if (std::abs(distance - geometry.radius) <= line_half_width) {
                    mark_pixel(
                        data,
                        static_cast<long>(x),
                        static_cast<long>(y),
                        radius_hint
                    );
                }
            }
        }

        const long center_x = std::lround(geometry.center_x);
        const long center_y = std::lround(geometry.center_y);
        const long radius_end_x = std::lround(std::min(
            geometry.center_x + geometry.radius,
            static_cast<double>(data.width - 1)
        ));
        for (long x = center_x; x <= radius_end_x; ++x) {
            for (long offset = -stroke_radius;
                 offset <= stroke_radius;
                 ++offset) {
                mark_pixel(data, x, center_y + offset, radius_hint);
            }
        }

        const long cross_arm = std::max(
            2L,
            std::lround(
                static_cast<double>(std::min(data.width, data.height)) / 50.0
            )
        );
        for (long offset = -cross_arm; offset <= cross_arm; ++offset) {
            for (long width_offset = -stroke_radius;
                 width_offset <= stroke_radius;
                 ++width_offset) {
                mark_pixel(
                    data,
                    center_x + offset,
                    center_y + width_offset,
                    center_hint
                );
                mark_pixel(
                    data,
                    center_x + width_offset,
                    center_y + offset,
                    center_hint
                );
            }
        }

        return data;
    }

    FileData apply_fisheye(
        FileData data,
        const FisheyeArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 || arguments.amount == 0.0) {
            return data;
        }

        const FisheyeGeometry geometry =
                calculate_geometry(data, arguments);
        if (geometry.radius == 0.0) {
            return data;
        }

        FileData result{
            .width = data.width,
            .height = data.height,
            .pixels = std::vector<Pixel>(data.pixels.size()),
        };

        for (std::size_t y = 0; y < data.height; ++y) {
            for (std::size_t x = 0; x < data.width; ++x) {
                const double offset_x =
                        static_cast<double>(x) - geometry.center_x;
                const double offset_y =
                        static_cast<double>(y) - geometry.center_y;
                const double distance = std::hypot(offset_x, offset_y);
                const std::size_t index = y * data.width + x;
                if (distance >= geometry.radius) {
                    result.pixels[index] = data.pixels[index];
                    continue;
                }

                const double angle = std::atan2(offset_y, offset_x);
                const double normalized_distance =
                        distance / geometry.radius;
                const double source_distance = distance / (
                    1.0 + arguments.amount * (1.0 - normalized_distance)
                );
                const double source_x =
                        geometry.center_x + source_distance * std::cos(angle);
                const double source_y =
                        geometry.center_y + source_distance * std::sin(angle);

                result.pixels[index] =
                        sample_bilinear(data, source_x, source_y);
            }
        }
        return result;
    }

    class FisheyeProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "fisheye";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                    processor_argument_parse::after_keyword(command, "fisheye");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_fisheye_arguments(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return apply_fisheye(
                std::move(data),
                parse_fisheye_arguments(arguments)
            );
        }

        [[nodiscard]] FileData add_debug_hints(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return add_fisheye_debug_hints(
                std::move(data),
                parse_fisheye_arguments(arguments)
            );
        }
    };
} // namespace

const ImageProcessor &fisheye_processor() {
    static const FisheyeProcessor processor;
    return processor;
}
