#ifndef MAGRITTE_BLACK_AND_WHITE_H
#define MAGRITTE_BLACK_AND_WHITE_H

#include "magritte/processors/image_processor.h"

/// Converts RGB to perceptual grayscale and scales the resulting luminance by
/// a nonnegative brightness multiplier. Alpha is preserved.
[[nodiscard]] const ImageProcessor &black_and_white_processor();

#endif // MAGRITTE_BLACK_AND_WHITE_H
