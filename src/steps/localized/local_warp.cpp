// Step: `local-warp = (<source-x>, <source-y>)`.
// Remaps every output pixel by bilinearly sampling the immutable input image.
// `source-x` and `source-y` are source-coordinate formulas that may also use
// `red(dx, dy)`, `green(dx, dy)`, and `blue(dx, dy)` to inspect neighboring
// immutable input pixels.

#include "magritte/steps/local_warp.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/steps/assignment_step.h"
#include "magritte/steps/utils/formula_apply.h"
#include "magritte/steps/utils/formula_parse.h"

namespace {
    class LocalWarpStep final : public AssignmentStep {
    public:
        LocalWarpStep()
            : AssignmentStep("local-warp") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "local warp formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_local_warp_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            const WarpFormula formula = parse_local_warp_formula(
                arguments,
                macros != nullptr ? *macros : empty_macros
            );
            return apply_warp_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageStep &local_warp_step() {
    static const LocalWarpStep step;
    return step;
}
