#ifndef PIXLIE_CONTRAST_H
#define PIXLIE_CONTRAST_H

#include "pixlie/processors/image_processor.h"

/// Scales each RGB channel's distance from the midpoint. A factor of 1 leaves
/// the image unchanged, while factors greater than 1 increase contrast.
[[nodiscard]] const ImageProcessor &contrast_processor();

#endif // PIXLIE_CONTRAST_H
