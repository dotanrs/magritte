#include "pixlie/processors/blue_formula.h"
#include "pixlie/processors/green_formula.h"
#include "pixlie/processors/red_formula.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/assignment_processor.h"
#include "pixlie/processors/utils/formula_apply.h"
#include "pixlie/processors/utils/formula_parse.h"

namespace {
    class ChannelFormulaProcessor final : public AssignmentProcessor {
    public:
        ChannelFormulaProcessor(
            std::string keyword,
            std::string name,
            ColorChannel channel
        )
            : AssignmentProcessor(std::move(keyword)),
              name_(std::move(name)),
              channel_(channel) {
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return name_;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_formula(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            const Formula formula = parse_formula(arguments);
            return apply_formula(std::move(data), *formula, channel_);
        }

    private:
        const std::string name_;
        const ColorChannel channel_;
    };
} // namespace

const ImageProcessor &red_formula_processor() {
    static const ChannelFormulaProcessor processor{
        "r",
        "red formula",
        ColorChannel::red,
    };
    return processor;
}

const ImageProcessor &green_formula_processor() {
    static const ChannelFormulaProcessor processor{
        "g",
        "green formula",
        ColorChannel::green,
    };
    return processor;
}

const ImageProcessor &blue_formula_processor() {
    static const ChannelFormulaProcessor processor{
        "b",
        "blue formula",
        ColorChannel::blue,
    };
    return processor;
}
