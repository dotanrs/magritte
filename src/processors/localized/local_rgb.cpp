// Processor: `local-rgb = (<red>, <green>, <blue>)`.
// Recomputes all RGB channels while preserving alpha. `red`, `green`, and
// `blue` are formulas for their corresponding output channels; in addition to
// normal formula variables, they may sample neighboring immutable input
// pixels with `red(dx, dy)`, `green(dx, dy)`, and `blue(dx, dy)`.

#include "magritte/processors/local_rgb.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/processors/assignment_processor.h"
#include "magritte/processors/utils/formula_apply.h"
#include "magritte/processors/utils/formula_parse.h"

namespace {

    class LocalRgbProcessor final : public AssignmentProcessor {
    public:
        LocalRgbProcessor()
            : AssignmentProcessor("local-rgb") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "local RGB formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_local_rgb_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const RgbFormula formula = parse_local_rgb_formula(arguments);
            return apply_local_rgb_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageProcessor &local_rgb_processor() {
    static const LocalRgbProcessor processor;
    return processor;
}
