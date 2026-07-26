#include "pixlie/processors/color_swap.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    void validate_channel(std::string_view channel) {
        if (channel != "r" && channel != "g" && channel != "b") {
            throw std::invalid_argument("color swap channels must be r, g, or b");
        }
    }

    void validate_arguments(const std::vector<std::string> &arguments) {
        if (arguments.size() != 2) {
            throw std::invalid_argument("color swap expects two channels");
        }

        validate_channel(arguments[0]);
        validate_channel(arguments[1]);
    }

    std::uint8_t &channel_value(Pixel &pixel, std::string_view channel) {
        if (channel == "r") {
            return pixel.red;
        }
        if (channel == "g") {
            return pixel.green;
        }
        return pixel.blue;
    }

    class ColorSwapProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "color swap";
        }

        void validate(const std::vector<std::string> &arguments) const override {
            validate_arguments(arguments);
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments
        ) const override {
            validate_arguments(arguments);

            for (Pixel &pixel: data.pixels) {
                std::swap(
                    channel_value(pixel, arguments[0]),
                    channel_value(pixel, arguments[1])
                );
            }
            return data;
        }
    };
} // namespace

const ImageProcessor &color_swap_processor() {
    static const ColorSwapProcessor processor;
    return processor;
}
