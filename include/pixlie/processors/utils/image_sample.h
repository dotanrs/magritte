#ifndef PIXLIE_IMAGE_SAMPLE_H
#define PIXLIE_IMAGE_SAMPLE_H

#include "../../common/file_data.h"

struct BilinearSample {
    double red;
    double green;
    double blue;
    double alpha;
};

/// Bilinearly samples channel values without rounding them to bytes.
/// Coordinates outside the image are clamped to its nearest edge.
[[nodiscard]] BilinearSample sample_bilinear_values(
    const FileData &data,
    double source_x,
    double source_y
);

/// Bilinearly samples `data` at floating-point source coordinates.
/// Coordinates outside the image are clamped to its nearest edge.
[[nodiscard]] Pixel sample_bilinear(
    const FileData &data,
    double source_x,
    double source_y
);

#endif // PIXLIE_IMAGE_SAMPLE_H
