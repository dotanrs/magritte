#include "pixlie/processors/rgb_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    class RgbFormulaProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "RGB formula";
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
