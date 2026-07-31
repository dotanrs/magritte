#ifndef MAGRITTE_LOOP_ASSIGNMENT_STEP_H
#define MAGRITTE_LOOP_ASSIGNMENT_STEP_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/steps/loop_step.h"
#include "magritte/steps/utils/argument_parse.h"

/// Reusable loop wrapper for steps with `<keyword> = <value>` syntax.
/// A future `loop-foo` can be registered by constructing this class with
/// `"foo"` and the existing foo step.
class LoopAssignmentStep final : public LoopStep {
public:
    LoopAssignmentStep(
        std::string substep_keyword,
        const ImageStep &substep
    )
        : LoopStep(substep),
          keyword_("loop-" + std::move(substep_keyword)) {
    }

    [[nodiscard]] std::string_view name() const noexcept final {
        return keyword_;
    }

    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const final {
        auto arguments =
            step_argument_parse::after_counted_assignment(
                command,
                keyword_
            );
        if (arguments) {
            validate(*arguments);
        }
        return arguments;
    }

private:
    const std::string keyword_;
};

#endif // MAGRITTE_LOOP_ASSIGNMENT_STEP_H
