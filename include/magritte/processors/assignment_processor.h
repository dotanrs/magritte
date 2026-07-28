#ifndef MAGRITTE_ASSIGNMENT_PROCESSOR_H
#define MAGRITTE_ASSIGNMENT_PROCESSOR_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/processors/image_processor.h"
#include "magritte/processors/utils/argument_parse.h"

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

#endif //MAGRITTE_ASSIGNMENT_PROCESSOR_H
