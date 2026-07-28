#ifndef MAGRITTE_TWIST_H
#define MAGRITTE_TWIST_H

#include "magritte/processors/image_processor.h"

/// Rotates source coordinates around a percentage-based center, with rotation
/// increasing linearly with distance from that center.
///
/// In debug mode, the processor marks its guide radius in yellow, its center
/// in magenta, and its force-shaped spin line in cyan.
[[nodiscard]] const ImageProcessor &twist_processor();

#endif // MAGRITTE_TWIST_H
