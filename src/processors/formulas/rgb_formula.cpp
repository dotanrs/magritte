// Processor: `<channels> [offset <x> <y>] = <formula-or-tuple>`.
// Recomputes a non-repeating target made from `r`, `g`, and `b` while
// preserving untargeted channels and alpha. `channels` gives the destination
// order; a one-channel target takes one formula, while a multi-channel target
// takes an equally sized tuple whose expressions all read the original pixel.
// The optional normalized offset replaces the image-center origin used by the
// polar variables A and D.

#include "magritte/processors/rgb_formula.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/processors/utils/argument_parse.h"
#include "magritte/processors/utils/formula_apply.h"
#include "magritte/processors/utils/formula_parse.h"

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

            const auto left_words = processor_argument_parse::split_words(
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
                if (left_words.size() != 4 || left_words[1] != "offset") {
                    throw std::invalid_argument(
                        "RGB formula offset expects two numbers: x y"
                    );
                }
                arguments.push_back(left_words[2]);
                arguments.push_back(left_words[3]);
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

const ImageProcessor &rgb_formula_processor() {
    static const RgbFormulaProcessor processor;
    return processor;
}
