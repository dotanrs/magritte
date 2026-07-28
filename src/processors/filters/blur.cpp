// Processor: `blur <radius>`.
// Applies an edge-truncated RGB box blur while preserving alpha. `radius` is
// the nonnegative integer neighborhood radius in pixels; zero is a no-op.

#include "magritte/processors/blur.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "magritte/processors/utils/argument_parse.h"

namespace {

using ChannelSums = std::array<std::uint64_t, 3>;

std::size_t parse_radius(const std::vector<std::string>& arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "blur expects exactly one nonnegative integer radius"
        );
    }

    const std::string_view value = arguments.front();
    int radius = 0;
    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), radius);
    if (error != std::errc{} || end != value.data() + value.size() ||
        radius < 0) {
        throw std::invalid_argument(
            "blur radius must be a nonnegative integer"
        );
    }
    return static_cast<std::size_t>(radius);
}

void add_pixel(ChannelSums& sums, const Pixel& pixel) {
    sums[0] += pixel.red;
    sums[1] += pixel.green;
    sums[2] += pixel.blue;
}

void subtract_pixel(ChannelSums& sums, const Pixel& pixel) {
    sums[0] -= pixel.red;
    sums[1] -= pixel.green;
    sums[2] -= pixel.blue;
}

void add_sums(ChannelSums& destination, const ChannelSums& source) {
    for (std::size_t channel = 0; channel < destination.size(); ++channel) {
        destination[channel] += source[channel];
    }
}

void subtract_sums(ChannelSums& destination, const ChannelSums& source) {
    for (std::size_t channel = 0; channel < destination.size(); ++channel) {
        destination[channel] -= source[channel];
    }
}

std::uint8_t rounded_average(std::uint64_t sum, std::size_t count) {
    return static_cast<std::uint8_t>(
        (sum + static_cast<std::uint64_t>(count / 2)) / count
    );
}

/// Applies an edge-truncated RGB box blur using rolling column and row sums.
/// The original alpha channel is left unchanged.
FileData apply_box_blur(FileData data, std::size_t radius) {
    if (radius == 0 || data.width == 0 || data.height == 0) {
        return data;
    }

    const std::size_t horizontal_radius = std::min(radius, data.width - 1);
    const std::size_t vertical_radius = std::min(radius, data.height - 1);
    std::vector<ChannelSums> column_sums(data.width);

    for (std::size_t y = 0; y <= vertical_radius; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            add_pixel(column_sums[x], data.pixels[y * data.width + x]);
        }
    }

    FileData blurred = data;
    for (std::size_t y = 0; y < data.height; ++y) {
        ChannelSums window_sums{};
        for (std::size_t x = 0; x <= horizontal_radius; ++x) {
            add_sums(window_sums, column_sums[x]);
        }

        const std::size_t first_y =
            y > vertical_radius ? y - vertical_radius : 0;
        const std::size_t last_y =
            std::min(data.height - 1, y + vertical_radius);
        const std::size_t row_count = last_y - first_y + 1;

        for (std::size_t x = 0; x < data.width; ++x) {
            const std::size_t first_x =
                x > horizontal_radius ? x - horizontal_radius : 0;
            const std::size_t last_x =
                std::min(data.width - 1, x + horizontal_radius);
            const std::size_t pixel_count =
                row_count * (last_x - first_x + 1);

            Pixel& output = blurred.pixels[y * data.width + x];
            output.red = rounded_average(window_sums[0], pixel_count);
            output.green = rounded_average(window_sums[1], pixel_count);
            output.blue = rounded_average(window_sums[2], pixel_count);

            if (x >= horizontal_radius) {
                subtract_sums(
                    window_sums,
                    column_sums[x - horizontal_radius]
                );
            }
            if (x + horizontal_radius + 1 < data.width) {
                add_sums(
                    window_sums,
                    column_sums[x + horizontal_radius + 1]
                );
            }
        }

        if (y >= vertical_radius) {
            const std::size_t removed_y = y - vertical_radius;
            for (std::size_t x = 0; x < data.width; ++x) {
                subtract_pixel(
                    column_sums[x],
                    data.pixels[removed_y * data.width + x]
                );
            }
        }
        if (y + vertical_radius + 1 < data.height) {
            const std::size_t added_y = y + vertical_radius + 1;
            for (std::size_t x = 0; x < data.width; ++x) {
                add_pixel(
                    column_sums[x],
                    data.pixels[added_y * data.width + x]
                );
            }
        }
    }
    return blurred;
}

class BlurProcessor final : public ImageProcessor {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "blur";
    }

    [[nodiscard]] std::optional<std::vector<std::string>> parse_arguments(
        std::string_view command
    ) const override {
        auto arguments =
            processor_argument_parse::after_keyword(command, "blur");
        if (arguments) {
            validate(*arguments);
        }
        return arguments;
    }

    void validate(const std::vector<std::string>& arguments) const override {
        static_cast<void>(parse_radius(arguments));
    }

    [[nodiscard]] FileData apply(
        FileData data,
        const std::vector<std::string>& arguments
    ) const override {
        return apply_box_blur(std::move(data), parse_radius(arguments));
    }
};

} // namespace

const ImageProcessor& blur_processor() {
    static const BlurProcessor processor;
    return processor;
}
