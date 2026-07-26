#ifndef PIXLIE_LIGHTING_H
#define PIXLIE_LIGHTING_H

#include "pixlie/processors/image_processor.h"

/// Applies colored directional light along parallel rays through their first
/// pixel whose luminance reaches the configured threshold.
[[nodiscard]] const ImageProcessor &lighting_processor();

#endif // PIXLIE_LIGHTING_H
