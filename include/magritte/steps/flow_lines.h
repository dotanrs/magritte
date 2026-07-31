#ifndef MAGRITTE_FLOW_LINES_H
#define MAGRITTE_FLOW_LINES_H

#include "magritte/steps/image_step.h"

/// Draws antialiased, occupancy-spaced streamlines through a formula-defined
/// two-dimensional vector field.
[[nodiscard]] const ImageStep &flow_lines_step();

#endif // MAGRITTE_FLOW_LINES_H
