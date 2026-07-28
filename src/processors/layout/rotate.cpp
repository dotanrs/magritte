// Processor: `rotate <turns>`.
// Rotates the image in 90-degree increments and updates its dimensions.
// `turns` is a signed integer reduced modulo four; positive values rotate
// clockwise and negative values rotate in the opposite direction.

#include "magritte/processors/rotate.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/processors/utils/argument_parse.h"

namespace {
    int parse_turns(const std::vector<std::string> &arguments) {
        if (arguments.size() != 1) {
            throw std::invalid_argument("rotate expects exactly one integer");
        }

        const std::string_view value = arguments.front();
        int turns = 0;
        const auto [end, error] =
                std::from_chars(value.data(), value.data() + value.size(), turns);
        if (error != std::errc{} || end != value.data() + value.size()) {
            throw std::invalid_argument("rotate argument must be an integer");
        }
        return turns;
    }

    FileData rotate_clockwise(FileData data) {
        FileData rotated{
            .width = data.height,
            .height = data.width,
            .pixels = std::vector<Pixel>(data.pixels.size()),
        };

        for (std::size_t y = 0; y < data.height; ++y) {
            for (std::size_t x = 0; x < data.width; ++x) {
                const std::size_t new_x = y;
                const std::size_t new_y = data.width - 1 - x;
                rotated.pixels[new_y * rotated.width + new_x] =
                        data.pixels[y * data.width + x];
            }
        }

        return rotated;
    }

    class RotateProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "rotate";
        }

        [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
            std::string_view command
        ) const override {
            auto arguments =
                processor_argument_parse::after_keyword(command, "rotate");
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(const std::vector<std::string> &arguments) const override {
            static_cast<void>(parse_turns(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            int turns = parse_turns(arguments) % 4;
            if (turns < 0) {
                turns += 4;
            }

            for (int turn = 0; turn < turns; ++turn) {
                data = rotate_clockwise(std::move(data));
            }
            return data;
        }
    };
} // namespace

const ImageProcessor &rotate_processor() {
    static const RotateProcessor processor;
    return processor;
}
