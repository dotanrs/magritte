#ifndef MAGRITTE_FISHEYE_H
#define MAGRITTE_FISHEYE_H

#include "magritte/processors/image_processor.h"

/// In debug mode, the fisheye processor marks its circular boundary and radius
/// in yellow, with a magenta crosshair at the center.
[[nodiscard]] const ImageProcessor &fisheye_processor();

#endif // MAGRITTE_FISHEYE_H
