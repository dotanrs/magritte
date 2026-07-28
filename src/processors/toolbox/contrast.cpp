#include "pixlie/processors/contrast.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/utils/argument_parse.h"

namespace {
    double parse_factor(const std::vector<std::string> &arguments) {
        if (arguments.size() != 1) {
            throw std::invalid_argument(
                "contrast expects exactly one factor"
            );
        }

        const std::string &text = arguments.front();
        char *end = nullptr;
        const double factor = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(factor)) {
            throw std::invalid_argument(
                "contrast factor must be a finite number"
            );
        }
        if (factor < 1.0) {
            throw std::invalid_argument(
                "contrast factor must be at least 1"
            );
        }
        return factor;
    }

    std::uint8_t adjust_channel(std::uint8_t channel, double factor) {
        constexpr double midpoint = 128.0;
        const double adjusted = std::clamp(
            midpoint +
                (static_cast<double>(channel) - midpoint) * factor,
            0.0,
            255.0
        );
        return static_cast<std::uint8_t>(std::lround(adjusted));
    }

    FileData apply_contrast(FileData data, double factor) {
        if (factor == 1.0) {
            return data;
        }

        for (Pixel &pixel: data.pixels) {
            pixel.red = adjust_channel(pixel.red, factor);
            pixel.green = adjust_channel(pixel.green, factor);
            pixel.blue = adjust_channel(pixel.blue, factor);
        }
        return data;
    }

    class ContrastProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "contrast";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_keyword(command, "contrast");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_factor(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            return apply_contrast(
                std::move(data),
                parse_factor(arguments)
            );
        }
    };
} // namespace

const ImageProcessor &contrast_processor() {
    static const ContrastProcessor processor;
    return processor;
}
