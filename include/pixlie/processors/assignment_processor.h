#ifndef PIXLIE_ASSIGNMENT_PROCESSOR_H
#define PIXLIE_ASSIGNMENT_PROCESSOR_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/image_processor.h"
#include "pixlie/processors/utils/argument_parse.h"

/// Base for processors whose commands use `<keyword> = <value>` syntax.
/// Matching assignments are validated before their right-hand side is returned.
class AssignmentProcessor : public ImageProcessor {
protected:
    explicit AssignmentProcessor(std::string keyword)
        : keyword_(std::move(keyword)) {
    }

public:
    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const final {
        auto arguments =
            processor_argument_parse::after_assignment(command, keyword_);
        if (arguments) {
            validate(*arguments);
        }
        return arguments;
    }

private:
    const std::string keyword_;
};

#endif //PIXLIE_ASSIGNMENT_PROCESSOR_H
