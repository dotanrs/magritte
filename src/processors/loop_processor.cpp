#include "pixlie/processors/loop_processor.h"

#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {
    struct ParsedLoopArguments {
        std::size_t iterations;
        std::vector<std::string> subprocessor_arguments;
    };

    ParsedLoopArguments parse_loop_arguments(
        const std::vector<std::string> &arguments
    ) {
        if (arguments.size() < 2) {
            throw std::invalid_argument(
                "loop processor expects an iteration count and "
                "subprocessor arguments"
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

LoopProcessor::LoopProcessor(const ImageProcessor &subprocessor)
    : subprocessor_(subprocessor) {
}

void LoopProcessor::validate(
    const std::vector<std::string> &arguments
) const {
    const auto parsed = parse_loop_arguments(arguments);
    subprocessor_.get().validate(parsed.subprocessor_arguments);
}

FileData LoopProcessor::apply(
    FileData data,
    const std::vector<std::string> &arguments
) const {
    const auto parsed = parse_loop_arguments(arguments);
    subprocessor_.get().validate(parsed.subprocessor_arguments);

    for (std::size_t iteration = 0;
         iteration < parsed.iterations;
         ++iteration) {
        data = subprocessor_.get().apply(
            std::move(data),
            parsed.subprocessor_arguments
        );
    }
    return data;
}

const ImageProcessor &LoopProcessor::subprocessor() const noexcept {
    return subprocessor_.get();
}
