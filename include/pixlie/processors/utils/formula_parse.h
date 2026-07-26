#ifndef PIXLIE_FORMULA_PARSE_H
#define PIXLIE_FORMULA_PARSE_H

#include <memory>
#include <string>
#include <vector>

enum class FormulaNodeKind {
    number,
    red,
    green,
    blue,
    saturation,
    x,
    y,
    width,
    height,
    normalized_x,
    normalized_y,
    distance,
    angle,
    add,
    subtract,
    multiply,
    divide,
    negate,
    sine,
    cosine,
    tangent,
    arc_tangent_2,
    square_root,
    power,
    modulo,
    absolute,
    minimum,
    maximum,
    clamp,
    floor,
    ceiling,
    round,
    exponential,
    logarithm,
};

struct FormulaNode {
    FormulaNodeKind kind;
    double number = 0.0;
    std::unique_ptr<FormulaNode> left;
    std::unique_ptr<FormulaNode> right;
    std::unique_ptr<FormulaNode> third;
};

using Formula = std::unique_ptr<FormulaNode>;

struct RgbFormula {
    Formula red;
    Formula green;
    Formula blue;
};

struct WarpFormula {
    Formula source_x;
    Formula source_y;
};

[[nodiscard]] Formula parse_formula(
    const std::vector<std::string> &arguments
);

[[nodiscard]] RgbFormula parse_rgb_formula(
    const std::vector<std::string> &arguments
);

[[nodiscard]] WarpFormula parse_warp_formula(
    const std::vector<std::string> &arguments
);

[[nodiscard]] Formula parse_saturation_formula(
    const std::vector<std::string> &arguments
);

#endif //PIXLIE_FORMULA_PARSE_H
