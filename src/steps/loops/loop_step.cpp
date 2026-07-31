// Shared step behavior for loop commands.
// Repeatedly applies a wrapped step, feeding each output into the next
// iteration. The first parsed argument is a nonnegative iteration count; all
// remaining arguments keep their wrapped step's meanings.

#include "magritte/steps/loop_step.h"

#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {
    struct ParsedLoopArguments {
        std::size_t iterations;
        std::vector<std::string> substep_arguments;
    };

    ParsedLoopArguments parse_loop_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() < 2) {
            throw std::invalid_argument(
                "loop step expects an iteration count and "
                "substep arguments"
            );
        }

        std::size_t iterations = 0;
        const std::string &count = arguments.front();
        const auto [end, error] = std::from_chars(
            count.data(),
            count.data() + count.size(),
            iterations
        );
        if (error != std::errc{} ||
            end != count.data() + count.size()) {
            throw std::invalid_argument(
                "loop iteration count must be a nonnegative integer"
            );
        }

        return ParsedLoopArguments{
            iterations,
            std::vector<std::string>(
                arguments.begin() + 1,
                arguments.end()
            )
        };
    }
} // namespace

LoopStep::LoopStep(const ImageStep &substep)
    : substep_(substep) {
}

void LoopStep::validate(
    const std::vector<std::string> &arguments
) const {
    const auto parsed = parse_loop_arguments(arguments);
    substep_.get().validate(parsed.substep_arguments);
}

FileData LoopStep::apply(
    FileData data,
    const std::vector<std::string> &arguments,
    const MacroMap *macros
) const {
    const auto parsed = parse_loop_arguments(arguments);
    substep_.get().validate(parsed.substep_arguments);

    for (std::size_t iteration = 0;
         iteration < parsed.iterations;
         ++iteration) {
        data = substep_.get().apply(
            std::move(data),
            parsed.substep_arguments,
            macros
        );
    }
    return data;
}

const ImageStep &LoopStep::substep() const noexcept {
    return substep_.get();
}
