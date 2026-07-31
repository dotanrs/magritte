#include "magritte/steps/utils/image_sample.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
    double bounded_coordinate(double value, std::size_t extent) {
        // Treat an unusable coordinate as the origin; otherwise clamp sampling
        // to the edge pixel instead of allowing a read outside the image.
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
        // Bilinear interpolation is two linear interpolations across the rows,
        // followed by one between those intermediate values.
        const double top =
                top_left + (top_right - top_left) * horizontal;
        const double bottom =
                bottom_left + (bottom_right - bottom_left) * horizontal;
        return top + (bottom - top) * vertical;
    }
} // namespace

// Image-warping steps work backwards: for each pixel in the output image,
// they calculate the corresponding (often fractional) location in the original
// image. Sampling converts that source location into a color. Blending the four
// surrounding pixels produces smooth results as the calculated location moves
// between pixel centers, instead of abruptly jumping to the nearest pixel.
//
// Formula evaluation uses this double-precision variant so sampled channel
// values can participate in further calculations without being rounded first.
BilinearSample sample_bilinear_values(
    const FileData &data,
    double source_x,
    double source_y
) {
    source_x = bounded_coordinate(source_x, data.width);
    source_y = bounded_coordinate(source_y, data.height);

    // The integer coordinates identify the four surrounding source pixels.
    // At the right and bottom edges, the neighbor collapses onto the edge pixel.
    const std::size_t left = static_cast<std::size_t>(std::floor(source_x));
    const std::size_t top = static_cast<std::size_t>(std::floor(source_y));
    const std::size_t right = std::min(left + 1, data.width - 1);
    const std::size_t bottom = std::min(top + 1, data.height - 1);

    // The fractional parts are the interpolation weights within this pixel cell.
    const double horizontal = source_x - static_cast<double>(left);
    const double vertical = source_y - static_cast<double>(top);

    // FileData stores pixels in row-major order.
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

// Geometric effects such as warp, twist, fisheye, and spin use this wrapper
// when they need a finished Pixel to place directly into the output image.
Pixel sample_bilinear(
    const FileData &data,
    double source_x,
    double source_y
) {
    const BilinearSample sampled =
            sample_bilinear_values(data, source_x, source_y);

    // Keep interpolation at double precision until the public Pixel boundary,
    // then round each channel to its nearest representable byte value.
    return Pixel{
        .red = static_cast<std::uint8_t>(std::lround(sampled.red)),
        .green = static_cast<std::uint8_t>(std::lround(sampled.green)),
        .blue = static_cast<std::uint8_t>(std::lround(sampled.blue)),
        .alpha = static_cast<std::uint8_t>(std::lround(sampled.alpha)),
    };
}
