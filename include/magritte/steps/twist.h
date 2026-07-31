#ifndef MAGRITTE_TWIST_H
#define MAGRITTE_TWIST_H

#include "magritte/steps/image_step.h"

/// Rotates source coordinates around a percentage-based center, with rotation
/// increasing linearly with distance from that center.
///
/// In debug mode, the step marks its guide radius in yellow, its center
/// in magenta, and its force-shaped spin line in cyan.
[[nodiscard]] const ImageStep &twist_step();

#endif // MAGRITTE_TWIST_H
