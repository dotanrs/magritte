#ifndef MAGRITTE_BLACK_AND_WHITE_H
#define MAGRITTE_BLACK_AND_WHITE_H

#include "magritte/steps/image_step.h"

/// Converts RGB to perceptual grayscale and scales the resulting luminance by
/// a nonnegative brightness multiplier. Alpha is preserved.
[[nodiscard]] const ImageStep &black_and_white_step();

#endif // MAGRITTE_BLACK_AND_WHITE_H
