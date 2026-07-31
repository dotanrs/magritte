// Step: `local-rgb = (<red>, <green>, <blue>)`.
// Recomputes all RGB channels while preserving alpha. `red`, `green`, and
// `blue` are formulas for their corresponding output channels; in addition to
// normal formula variables, they may sample neighboring immutable input
// pixels with `red(dx, dy)`, `green(dx, dy)`, and `blue(dx, dy)`.

#include "magritte/steps/local_rgb.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/steps/assignment_step.h"
#include "magritte/steps/utils/formula_apply.h"
#include "magritte/steps/utils/formula_parse.h"

namespace {

    class LocalRgbStep final : public AssignmentStep {
    public:
        LocalRgbStep()
            : AssignmentStep("local-rgb") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "local RGB formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_local_rgb_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            const RgbFormula formula = parse_local_rgb_formula(
                arguments,
                macros != nullptr ? *macros : empty_macros
            );
            return apply_local_rgb_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageStep &local_rgb_step() {
    static const LocalRgbStep step;
    return step;
}
