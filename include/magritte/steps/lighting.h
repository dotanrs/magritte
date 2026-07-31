#ifndef MAGRITTE_LIGHTING_H
#define MAGRITTE_LIGHTING_H

#include "magritte/steps/image_step.h"

/// Applies colored directional light along parallel rays through their first
/// formulas whose luminance reaches the configured threshold.
[[nodiscard]] const ImageStep &lighting_step();

#endif // MAGRITTE_LIGHTING_H
