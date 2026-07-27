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
    sample_red,
    sample_green,
    sample_blue,
};

struct FormulaNode {
    FormulaNodeKind kind;
    double number = 0.0;
    std::unique_ptr<FormulaNode> left;
    std::unique_ptr<FormulaNode> right;
    std::unique_ptr<FormulaNode> third;
};

using Formula = std::unique_ptr<FormulaNode>;

enum class ColorChannel {
    red,
    green,
    blue,
};

struct RgbFormula {
    std::vector<ColorChannel> channels;
    std::vector<Formula> expressions;
};

struct WarpFormula {
    Formula source_x;
    Formula source_y;
};

/// Parses either a fixed RGB tuple from one argument or a channel target and
/// its matching expression(s) from two arguments. Multi-channel formulas use
/// a parenthesized tuple; a single-channel formula is one expression.
/// @throws std::invalid_argument for an invalid target or formula.
[[nodiscard]] RgbFormula parse_rgb_formula(
    const std::vector<std::string> &arguments
);

/// Parses a parenthesized `(red, green, blue)` expression tuple in the local
/// RGB dialect, which additionally supports `red(dx, dy)`, `green(dx, dy)`,
/// and `blue(dx, dy)` source-image sampling functions.
/// @throws std::invalid_argument for a missing argument or invalid tuple.
[[nodiscard]] RgbFormula parse_local_rgb_formula(
    const std::vector<std::string> &arguments
);

/// Parses a parenthesized `(source_x, source_y)` expression pair.
/// @throws std::invalid_argument for a missing argument or invalid pair.
[[nodiscard]] WarpFormula parse_warp_formula(
    const std::vector<std::string> &arguments
);

/// Parses an expression in the saturation dialect, where `S` replaces the RGB
/// channel variables.
/// @throws std::invalid_argument for a missing argument or invalid expression.
[[nodiscard]] Formula parse_saturation_formula(
    const std::vector<std::string> &arguments
);

#endif //PIXLIE_FORMULA_PARSE_H
