// Step: `black-and-white <brightness>`.
// Converts RGB to perceptual grayscale while preserving alpha. `brightness`
// is a nonnegative finite multiplier applied to luminance before clamping;
// 1 preserves the computed luminance and 0 produces black.

#include "magritte/steps/black_and_white.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/steps/utils/argument_parse.h"

namespace {
    double parse_brightness_multiplier(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() != 1) {
            return 1; // Default multiplier
        }

        const std::string &text = arguments.front();
        char *end = nullptr;
        const double multiplier = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() ||
            !std::isfinite(multiplier)) {
            throw std::invalid_argument(
                "black-and-white brightness multiplier must be a finite number"
            );
        }
        if (multiplier < 0.0) {
            throw std::invalid_argument(
                "black-and-white brightness multiplier must be nonnegative"
            );
        }
        return multiplier;
    }

    std::uint8_t grayscale_value(
        const Pixel &pixel,
        double brightness_multiplier
    ) {
        constexpr double red_weight = 0.299;
        constexpr double green_weight = 0.587;
        constexpr double blue_weight = 0.114;

        const double luminance =
            red_weight * static_cast<double>(pixel.red) +
            green_weight * static_cast<double>(pixel.green) +
            blue_weight * static_cast<double>(pixel.blue);
        const double adjusted = luminance * brightness_multiplier;
        if (adjusted >= 255.0) {
            return 255;
        }
        return static_cast<std::uint8_t>(std::lround(adjusted));
    }

    FileData apply_black_and_white(
        FileData data,
        double brightness_multiplier
    ) {
        for (Pixel &pixel: data.pixels) {
            const std::uint8_t grayscale =
                grayscale_value(pixel, brightness_multiplier);
            pixel.red = grayscale;
            pixel.green = grayscale;
            pixel.blue = grayscale;
        }
        return data;
    }

    class BlackAndWhiteStep final : public ImageStep {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "black-and-white";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments = step_argument_parse::after_keyword(
                command,
                "black-and-white"
            );
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_brightness_multiplier(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *
        ) const override {
            return apply_black_and_white(
                std::move(data),
                parse_brightness_multiplier(arguments)
            );
        }
    };
} // namespace

const ImageStep &black_and_white_step() {
    static const BlackAndWhiteStep step;
    return step;
}
