#ifndef PIXLIE_FORMULA_APPLY_H
#define PIXLIE_FORMULA_APPLY_H

#include "pixlie/processor.h"
#include "pixlie/processors/utils/formula_parse.h"

enum class ColorChannel {
    red,
    green,
    blue,
};

/// Evaluates a formula for every pixel and replaces only `channel`.
/// Results are rounded and clamped to the byte range; NaN becomes zero.
[[nodiscard]] FileData apply_formula(
    FileData data,
    const FormulaNode &formula,
    ColorChannel channel
);

/// Replaces all RGB channels, evaluating every expression against the same
/// original value of each pixel. Alpha is preserved.
[[nodiscard]] FileData apply_rgb_formula(
    FileData data,
    const RgbFormula &formula
);

/// Replaces all RGB channels using expressions that may sample the unmodified
/// source image with relative, bilinearly interpolated coordinates. Alpha is
/// preserved from the current source pixel.
[[nodiscard]] FileData apply_local_rgb_formula(
    FileData data,
    const RgbFormula &formula
);

/// Remaps each output pixel from the unmodified input using bilinear sampling.
/// Source coordinates outside the image are clamped to its nearest edge.
[[nodiscard]] FileData apply_warp_formula(
    FileData data,
    const WarpFormula &formula
);

/// Replaces HSL saturation from a formula while preserving hue, lightness, and
/// alpha. Formula saturation values use the byte range `[0, 255]`.
[[nodiscard]] FileData apply_saturation_formula(
    FileData data,
    const FormulaNode &formula
);

#endif //PIXLIE_FORMULA_APPLY_H
