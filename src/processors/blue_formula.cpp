#include "pixlie/processors/blue_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/argument_parse.h"
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    class BlueFormulaProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "blue formula";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_assignment(command, "b");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
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
                ColorChannel::blue
            );
        }
    };
} // namespace

const ImageProcessor &blue_formula_processor() {
    static const BlueFormulaProcessor processor;
    return processor;
}
