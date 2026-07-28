#ifndef MAGRITTE_LIGHTING_H
#define MAGRITTE_LIGHTING_H

#include "magritte/processors/image_processor.h"

/// Applies colored directional light along parallel rays through their first
/// formulas whose luminance reaches the configured threshold.
[[nodiscard]] const ImageProcessor &lighting_processor();

#endif // MAGRITTE_LIGHTING_H
