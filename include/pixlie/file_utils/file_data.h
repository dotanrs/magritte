#ifndef PIXLIE_FILE_DATA_H
#define PIXLIE_FILE_DATA_H

#include <cstddef>
#include <cstdint>
#include <vector>

struct Pixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

static_assert(sizeof(Pixel) == 4);

/// An image stored as tightly packed, top-to-bottom rows of RGBA pixels.
/// A valid buffer contains exactly `width * height` pixels.
struct FileData {
    std::size_t width;
    std::size_t height;
    std::vector<Pixel> pixels;
};

#endif // PIXLIE_FILE_DATA_H
