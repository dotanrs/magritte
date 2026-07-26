#ifndef PIXLIE_FORMULA_APPLY_H
#define PIXLIE_FORMULA_APPLY_H

#include "pixlie/processor.h"
#include "pixlie/processors/utils/formula_parse.h"

enum class ColorChannel {
    red,
    green,
    blue,
};

[[nodiscard]] FileData apply_formula(
    FileData data,
    const FormulaNode &formula,
    ColorChannel channel
);

[[nodiscard]] FileData apply_rgb_formula(
    FileData data,
    const RgbFormula &formula
);

[[nodiscard]] FileData apply_saturation_formula(
    FileData data,
    const FormulaNode &formula
);

#endif //PIXLIE_FORMULA_APPLY_H
