#ifndef MAGRITTE_CONTRAST_H
#define MAGRITTE_CONTRAST_H

#include "magritte/steps/image_step.h"

/// Scales each RGB channel's distance from the midpoint. A factor of 1 leaves
/// the image unchanged, while factors greater than 1 increase contrast.
[[nodiscard]] const ImageStep &contrast_step();

#endif // MAGRITTE_CONTRAST_H
