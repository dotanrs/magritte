#include "pixlie/processors/green_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    class GreenFormulaProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "green formula";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const Formula formula = parse_formula(arguments);
            return apply_formula(
                std::move(data),
                *formula,
                ColorChannel::green
            );
        }
    };
} // namespace

const ImageProcessor &green_formula_processor() {
    static const GreenFormulaProcessor processor;
    return processor;
}
