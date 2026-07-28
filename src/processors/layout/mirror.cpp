// Processor: `mirror <axis>`.
// Reflects the image without changing its dimensions. `axis` must be `x` to
// exchange left and right pixels, or `y` to exchange top and bottom pixels.

#include "pixlie/processors/mirror.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "pixlie/processors/utils/argument_parse.h"

namespace {

std::string_view parse_axis(const std::vector<std::string>& arguments) {
    if (arguments.size() != 1 ||
        (arguments.front() != "x" && arguments.front() != "y")) {
        throw std::invalid_argument("mirror expects exactly one axis: x or y");
    }
    return arguments.front();
}

void mirror_vertical(FileData& data) {
    for (std::size_t y = 0; y < data.height / 2; ++y) {
        const std::size_t opposite_y = data.height - 1 - y;
        for (std::size_t x = 0; x < data.width; ++x) {
            std::swap(
                data.pixels[y * data.width + x],
                data.pixels[opposite_y * data.width + x]
            );
        }
    }
}

void mirror_horizontal(FileData& data) {
    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width / 2; ++x) {
            std::swap(
                data.pixels[y * data.width + x],
                data.pixels[y * data.width + (data.width - 1 - x)]
            );
        }
    }
}

class MirrorProcessor final : public ImageProcessor {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "mirror";
    }

    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const override {
        auto arguments =
            processor_argument_parse::after_keyword(command, "mirror");
        if (arguments) {
            validate(*arguments);
        }
        return arguments;
    }

    void validate(const std::vector<std::string>& arguments) const override {
        static_cast<void>(parse_axis(arguments));
    }

    [[nodiscard]] FileData apply(
        FileData data,
        const std::vector<std::string>& arguments
    ) const override {
        if (parse_axis(arguments) == "x") {
            mirror_horizontal(data);
        } else {
            mirror_vertical(data);
        }
        return data;
    }
};

} // namespace

const ImageProcessor& mirror_processor() {
    static const MirrorProcessor processor;
    return processor;
}
