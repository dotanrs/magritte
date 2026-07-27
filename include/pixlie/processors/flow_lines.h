#ifndef PIXLIE_FLOW_LINES_H
#define PIXLIE_FLOW_LINES_H

#include "pixlie/processors/image_processor.h"

/// Draws antialiased, occupancy-spaced streamlines through a formula-defined
/// two-dimensional vector field.
[[nodiscard]] const ImageProcessor &flow_lines_processor();

#endif // PIXLIE_FLOW_LINES_H
