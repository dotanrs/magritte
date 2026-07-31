// Step: `s = <formula>`.
// Recomputes each pixel's HSL saturation while preserving hue, lightness, and
// alpha. `formula` produces saturation on the 0-255 scale and may use the
// current saturation `S`, image coordinates, constants, and math functions.

#include "magritte/steps/saturation_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/steps/assignment_step.h"
#include "magritte/steps/utils/formula_apply.h"
#include "magritte/steps/utils/formula_parse.h"

namespace {
    class SaturationFormulaStep final : public AssignmentStep {
    public:
        SaturationFormulaStep()
            : AssignmentStep("s") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "saturation formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_saturation_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            const Formula formula = parse_saturation_formula(
                arguments,
                macros != nullptr ? *macros : empty_macros
            );
            return apply_saturation_formula(std::move(data), *formula);
        }
    };
} // namespace

const ImageStep &saturation_formula_step() {
    static const SaturationFormulaStep step;
    return step;
}
