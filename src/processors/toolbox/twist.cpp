#include "pixlie/processors/twist.h"

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
    struct TwistArguments {
        double center_x_percent;
        double center_y_percent;
        double force;
    };

    double parse_number(const std::string &text, std::string_view name) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "twist " + std::string(name) + " must be a finite number"
            );
        }
        return value;
    }

    TwistArguments parse_twist_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() != 3) {
            throw std::invalid_argument(
                "twist expects three numbers: x y force"
            );
        }

        TwistArguments result{
            .center_x_percent = parse_number(arguments[0], "x"),
            .center_y_percent = parse_number(arguments[1], "y"),
            .force = parse_number(arguments[2], "force"),
        };
        if (result.center_x_percent < 0.0 ||
            result.center_x_percent > 100.0 ||
            result.center_y_percent < 0.0 ||
            result.center_y_percent > 100.0) {
            throw std::invalid_argument(
                "twist x and y must be percentages from 0 to 100"
            );
        }
        return result;
    }

    FileData apply_twist(
        FileData data,
        const TwistArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 || arguments.force == 0.0) {
            return data;
        }

        const double center_x =
            static_cast<double>(data.width - 1) *
            arguments.center_x_percent / 100.0;
        const double center_y =
            static_cast<double>(data.height - 1) *
            arguments.center_y_percent / 100.0;
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
                const double source_angle =
                    std::atan2(offset_y, offset_x) +
                    distance * arguments.force / 100.0;
                const double source_x =
                    center_x + distance * std::cos(source_angle);
                const double source_y =
                    center_y + distance * std::sin(source_angle);

                result.pixels[y * data.width + x] =
                    sample_bilinear(data, source_x, source_y);
            }
        }
        return result;
    }

    class TwistProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "twist";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_keyword(command, "twist");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_twist_arguments(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return apply_twist(
                std::move(data),
                parse_twist_arguments(arguments)
            );
        }
    };
} // namespace

const ImageProcessor &twist_processor() {
    static const TwistProcessor processor;
    return processor;
}
