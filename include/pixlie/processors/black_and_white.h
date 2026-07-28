#ifndef PIXLIE_BLACK_AND_WHITE_H
#define PIXLIE_BLACK_AND_WHITE_H

#include "pixlie/processors/image_processor.h"

/// Converts RGB to perceptual grayscale and scales the resulting luminance by
/// a nonnegative brightness multiplier. Alpha is preserved.
[[nodiscard]] const ImageProcessor &black_and_white_processor();

#endif // PIXLIE_BLACK_AND_WHITE_H
