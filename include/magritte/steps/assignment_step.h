#ifndef MAGRITTE_ASSIGNMENT_STEP_H
#define MAGRITTE_ASSIGNMENT_STEP_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/steps/image_step.h"
#include "magritte/steps/utils/argument_parse.h"

/// Base for steps whose commands use `<keyword> = <value>` syntax.
/// Matching assignments are validated before their right-hand side is returned.
class AssignmentStep : public ImageStep {
protected:
    explicit AssignmentStep(std::string keyword)
        : keyword_(std::move(keyword)) {
    }

public:
    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const final {
        auto arguments =
            step_argument_parse::after_assignment(command, keyword_);
        if (arguments) {
            validate(*arguments);
        }
        return arguments;
    }

private:
    const std::string keyword_;
};

#endif //MAGRITTE_ASSIGNMENT_STEP_H
