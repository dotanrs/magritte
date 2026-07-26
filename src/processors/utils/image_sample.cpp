#include "pixlie/processors/utils/image_sample.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
    double bounded_coordinate(double value, std::size_t extent) {
        if (extent == 0 || std::isnan(value)) {
            return 0.0;
        }
        return std::clamp(value, 0.0, static_cast<double>(extent - 1));
    }

    double interpolate_channel(
        std::uint8_t top_left,
        std::uint8_t top_right,
        std::uint8_t bottom_left,
        std::uint8_t bottom_right,
        double horizontal,
        double vertical
    ) {
        const double top =
                top_left + (top_right - top_left) * horizontal;
        const double bottom =
                bottom_left + (bottom_right - bottom_left) * horizontal;
        return top + (bottom - top) * vertical;
    }
} // namespace

BilinearSample sample_bilinear_values(
    const FileData &data,
    double source_x,
    double source_y
) {
    source_x = bounded_coordinate(source_x, data.width);
    source_y = bounded_coordinate(source_y, data.height);

    const std::size_t left = static_cast<std::size_t>(std::floor(source_x));
    const std::size_t top = static_cast<std::size_t>(std::floor(source_y));
    const std::size_t right = std::min(left + 1, data.width - 1);
    const std::size_t bottom = std::min(top + 1, data.height - 1);
    const double horizontal = source_x - static_cast<double>(left);
    const double vertical = source_y - static_cast<double>(top);

    const Pixel &top_left = data.pixels[top * data.width + left];
    const Pixel &top_right = data.pixels[top * data.width + right];
    const Pixel &bottom_left = data.pixels[bottom * data.width + left];
    const Pixel &bottom_right = data.pixels[bottom * data.width + right];

    return BilinearSample{
        .red = interpolate_channel(
            top_left.red,
            top_right.red,
            bottom_left.red,
            bottom_right.red,
            horizontal,
            vertical
        ),
        .green = interpolate_channel(
            top_left.green,
            top_right.green,
            bottom_left.green,
            bottom_right.green,
            horizontal,
            vertical
        ),
        .blue = interpolate_channel(
            top_left.blue,
            top_right.blue,
            bottom_left.blue,
            bottom_right.blue,
            horizontal,
            vertical
        ),
        .alpha = interpolate_channel(
            top_left.alpha,
            top_right.alpha,
            bottom_left.alpha,
            bottom_right.alpha,
            horizontal,
            vertical
        ),
    };
}

Pixel sample_bilinear(
    const FileData &data,
    double source_x,
    double source_y
) {
    const BilinearSample sampled =
            sample_bilinear_values(data, source_x, source_y);
    return Pixel{
        .red = static_cast<std::uint8_t>(std::lround(sampled.red)),
        .green = static_cast<std::uint8_t>(std::lround(sampled.green)),
        .blue = static_cast<std::uint8_t>(std::lround(sampled.blue)),
        .alpha = static_cast<std::uint8_t>(std::lround(sampled.alpha)),
    };
}
