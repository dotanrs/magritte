#include "pixlie/processors/local_warp.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/assignment_processor.h"
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

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
