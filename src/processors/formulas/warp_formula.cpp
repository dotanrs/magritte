// Processor: `warp = (<source-x>, <source-y>)`.
// Remaps every output pixel by bilinearly sampling the input image.
// `source-x` and `source-y` are formulas that return the input coordinates to
// sample for the current output coordinate.

#include "magritte/processors/warp_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/processors/assignment_processor.h"
#include "magritte/processors/utils/formula_apply.h"
#include "magritte/processors/utils/formula_parse.h"

namespace {
    class WarpFormulaProcessor final : public AssignmentProcessor {
    public:
        WarpFormulaProcessor()
            : AssignmentProcessor("warp") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "warp formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_warp_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            const WarpFormula formula = parse_warp_formula(
                arguments,
                macros != nullptr ? *macros : empty_macros
            );
            return apply_warp_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageProcessor &warp_formula_processor() {
    static const WarpFormulaProcessor processor;
    return processor;
}
