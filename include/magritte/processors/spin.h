#ifndef MAGRITTE_SPIN_H
#define MAGRITTE_SPIN_H

#include "magritte/processors/image_processor.h"

/// Rotates source coordinates by a fixed angle around a percentage-based
/// center, optionally limiting the effect to a circular radius.
///
/// In debug mode, the processor marks its guide radius in yellow, its center
/// in magenta, and its angle line in cyan.
[[nodiscard]] const ImageProcessor &spin_processor();

#endif // MAGRITTE_SPIN_H
