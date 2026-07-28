// Processor: `s = <formula>`.
// Recomputes each pixel's HSL saturation while preserving hue, lightness, and
// alpha. `formula` produces saturation on the 0-255 scale and may use the
// current saturation `S`, image coordinates, constants, and math functions.

#include "pixlie/processors/saturation_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/assignment_processor.h"
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    class SaturationFormulaProcessor final : public AssignmentProcessor {
    public:
        SaturationFormulaProcessor()
            : AssignmentProcessor("s") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "saturation formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_saturation_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const Formula formula = parse_saturation_formula(arguments);
            return apply_saturation_formula(std::move(data), *formula);
        }
    };
} // namespace

const ImageProcessor &saturation_formula_processor() {
    static const SaturationFormulaProcessor processor;
    return processor;
}
