#ifndef MAGRITTE_FLOW_LINES_H
#define MAGRITTE_FLOW_LINES_H

#include "magritte/processors/image_processor.h"

/// Draws antialiased, occupancy-spaced streamlines through a formula-defined
/// two-dimensional vector field.
[[nodiscard]] const ImageProcessor &flow_lines_processor();

#endif // MAGRITTE_FLOW_LINES_H
