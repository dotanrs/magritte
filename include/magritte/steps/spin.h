#ifndef MAGRITTE_SPIN_H
#define MAGRITTE_SPIN_H

#include "magritte/steps/image_step.h"

/// Rotates source coordinates by a fixed angle around a percentage-based
/// center, optionally limiting the effect to a circular radius.
///
/// In debug mode, the step marks its guide radius in yellow, its center
/// in magenta, and its angle line in cyan.
[[nodiscard]] const ImageStep &spin_step();

#endif // MAGRITTE_SPIN_H
