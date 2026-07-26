#ifndef PIXLIE_LOOP_ASSIGNMENT_PROCESSOR_H
#define PIXLIE_LOOP_ASSIGNMENT_PROCESSOR_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pixlie/processors/loop_processor.h"
#include "pixlie/processors/utils/argument_parse.h"

/// Reusable loop wrapper for processors with `<keyword> = <value>` syntax.
/// A future `loop-foo` can be registered by constructing this class with
/// `"foo"` and the existing foo processor.
class LoopAssignmentProcessor final : public LoopProcessor {
public:
    LoopAssignmentProcessor(
        std::string subprocessor_keyword,
        const ImageProcessor &subprocessor
    )
        : LoopProcessor(subprocessor),
          keyword_("loop-" + std::move(subprocessor_keyword)) {
    }

    [[nodiscard]] std::string_view name() const noexcept final {
        return keyword_;
    }

    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const final {
        auto arguments =
            processor_argument_parse::after_counted_assignment(
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

#endif // PIXLIE_LOOP_ASSIGNMENT_PROCESSOR_H
