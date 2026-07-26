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

    FileData apply_fisheye(
        FileData data,
        const FisheyeArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 || arguments.amount == 0.0) {
            return data;
        }

        const double last_x = static_cast<double>(data.width - 1);
        const double last_y = static_cast<double>(data.height - 1);
        const double center_x =
                last_x * arguments.center_x_percent / 100.0;
        const double center_y =
                last_y * arguments.center_y_percent / 100.0;
        const double radius =
                static_cast<double>(std::min(data.width, data.height)) *
                arguments.radius_percent / 100.0;
        if (!std::isfinite(radius)) {
            throw std::invalid_argument("fisheye radius is too large");
        }
        if (radius == 0.0) {
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
                        static_cast<double>(x) - center_x;
                const double offset_y =
                        static_cast<double>(y) - center_y;
                const double distance = std::hypot(offset_x, offset_y);
                const std::size_t index = y * data.width + x;
                if (distance >= radius) {
                    result.pixels[index] = data.pixels[index];
                    continue;
                }

                const double angle = std::atan2(offset_y, offset_x);
                const double normalized_distance = distance / radius;
                const double source_distance = distance / (
                    1.0 + arguments.amount * (1.0 - normalized_distance)
                );
                const double source_x =
                        center_x + source_distance * std::cos(angle);
                const double source_y =
                        center_y + source_distance * std::sin(angle);

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
    };
} // namespace

const ImageProcessor &fisheye_processor() {
    static const FisheyeProcessor processor;
    return processor;
}
