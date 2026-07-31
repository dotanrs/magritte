#ifndef MAGRITTE_FISHEYE_H
#define MAGRITTE_FISHEYE_H

#include "magritte/steps/image_step.h"

/// In debug mode, the fisheye step marks its circular boundary and radius
/// in yellow, with a magenta crosshair at the center.
[[nodiscard]] const ImageStep &fisheye_step();

#endif // MAGRITTE_FISHEYE_H
