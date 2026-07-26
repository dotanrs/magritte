#ifndef PIXLIE_IMAGE_SAMPLE_H
#define PIXLIE_IMAGE_SAMPLE_H

#include "pixlie/file_utils/file_data.h"

/// Bilinearly samples `data` at floating-point source coordinates.
/// Coordinates outside the image are clamped to its nearest edge.
[[nodiscard]] Pixel sample_bilinear(
    const FileData &data,
    double source_x,
    double source_y
);

#endif // PIXLIE_IMAGE_SAMPLE_H
