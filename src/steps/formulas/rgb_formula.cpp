// Step: `<channels> [offset <x> <y> [radius]] = <formula-or-tuple>`.
// Recomputes a non-repeating target made from `r`, `g`, and `b` while
// preserving untargeted channels and alpha. `channels` gives the destination
// order; a one-channel target takes one formula, while a multi-channel target
// takes an equally sized tuple whose expressions all read the original pixel.
// The optional percentage offset replaces the image-center origin used by the
// polar variables A and D. When a radius percentage is supplied, the formula
// only changes pixels inside that circle.

#include "magritte/steps/rgb_formula.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/steps/utils/argument_parse.h"
#include "magritte/steps/utils/formula_apply.h"
#include "magritte/steps/utils/formula_parse.h"

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

    class RgbFormulaStep final : public ImageStep {
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

            const auto left_words = step_argument_parse::split_words(
                trim(value.substr(0, equals))
            );
            if (left_words.empty() || !is_rgb_target(left_words.front())) {
                return std::nullopt;
            }

            std::vector<std::string> arguments{
                left_words.front(),
                std::string(trim(value.substr(equals + 1))),
            };
            if (left_words.size() != 1) {
                if ((left_words.size() != 4 && left_words.size() != 5) ||
                    left_words[1] != "offset") {
                    throw std::invalid_argument(
                        "RGB formula offset expects two or three numbers: "
                        "x y [radius]"
                    );
                }
                arguments.push_back(left_words[2]);
                arguments.push_back(left_words[3]);
                if (left_words.size() == 5) {
                    arguments.push_back(left_words[4]);
                }
            }
            validate(arguments);
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_rgb_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            const RgbFormula formula = parse_rgb_formula(
                arguments,
                macros != nullptr ? *macros : empty_macros
            );
            return apply_rgb_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageStep &rgb_formula_step() {
    static const RgbFormulaStep step;
    return step;
}
