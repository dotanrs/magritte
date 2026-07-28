// Processor: `local-warp = (<source-x>, <source-y>)`.
// Remaps every output pixel by bilinearly sampling the immutable input image.
// `source-x` and `source-y` are source-coordinate formulas that may also use
// `red(dx, dy)`, `green(dx, dy)`, and `blue(dx, dy)` to inspect neighboring
// immutable input pixels.

#include "magritte/processors/local_warp.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/processors/assignment_processor.h"
#include "magritte/processors/utils/formula_apply.h"
#include "magritte/processors/utils/formula_parse.h"

namespace {
    class LocalWarpProcessor final : public AssignmentProcessor {
    public:
        LocalWarpProcessor()
            : AssignmentProcessor("local-warp") {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "local warp formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_local_warp_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const WarpFormula formula = parse_local_warp_formula(arguments);
            return apply_warp_formula(std::move(data), formula);
        }
    };
} // namespace

const ImageProcessor &local_warp_processor() {
    static const LocalWarpProcessor processor;
    return processor;
}
