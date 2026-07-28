// Processor: `<channels> = <formula-or-tuple>`.
// Recomputes a non-repeating target made from `r`, `g`, and `b` while
// preserving untargeted channels and alpha. `channels` gives the destination
// order; a one-channel target takes one formula, while a multi-channel target
// takes an equally sized tuple whose expressions all read the original pixel.

#include "pixlie/processors/rgb_formula.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    std::string_view trim(std::string_view value) {
        while (!value.empty() &&
               std::isspace(static_cast<unsigned char>(value.front())) != 0) {
            value.remove_prefix(1);
        }
        while (!value.empty() &&
               std::isspace(static_cast<unsigned char>(value.back())) != 0) {
            value.remove_suffix(1);
        }
        return value;
    }

    bool is_rgb_target(std::string_view value) {
        return !value.empty() &&
               std::all_of(
                   value.begin(),
                   value.end(),
                   [](char channel) {
                       return channel == 'r' ||
                              channel == 'g' ||
                              channel == 'b';
                   }
               );
    }

    class RgbFormulaProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "RGB formula";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            const std::string_view value = trim(command);
            const std::size_t equals = value.find('=');
            if (equals == std::string_view::npos) {
                return std::nullopt;
            }

            const std::string_view target = trim(value.substr(0, equals));
            if (!is_rgb_target(target)) {
                return std::nullopt;
            }

            std::vector<std::string> arguments{
                std::string(target),
                std::string(trim(value.substr(equals + 1))),
            };
            validate(arguments);
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_rgb_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const RgbFormula formula = parse_rgb_formula(arguments);
            return apply_rgb_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageProcessor &rgb_formula_processor() {
    static const RgbFormulaProcessor processor;
    return processor;
}
