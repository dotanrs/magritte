#include "pixlie/processors/local_rgb.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/assignment_processor.h"
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

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
