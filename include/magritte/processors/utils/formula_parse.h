#ifndef MAGRITTE_FORMULA_PARSE_H
#define MAGRITTE_FORMULA_PARSE_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "magritte/macro.h"

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

/// Percentage image coordinates for the origin used by polar variables, plus
/// an optional circular application radius measured against the shorter image
/// dimension.
struct FormulaPolarOrigin {
    double x_percent;
    double y_percent;
    std::optional<double> radius_percent;
};

struct RgbFormula {
    std::vector<ColorChannel> channels;
    std::vector<Formula> expressions;
    std::optional<FormulaPolarOrigin> polar_origin;
};

struct WarpFormula {
    Formula source_x;
    Formula source_y;
};

struct VectorFormula {
    Formula x;
    Formula y;
};

/// Parses either a fixed RGB tuple from one argument, a channel target and its
/// matching expression(s) from two arguments, or those two arguments followed
/// by polar-origin x and y percentages and an optional radius percentage.
/// Multi-channel formulas use a parenthesized tuple; a single-channel formula
/// is one expression.
/// @throws std::invalid_argument for an invalid target or formula.
[[nodiscard]] RgbFormula parse_rgb_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] RgbFormula parse_rgb_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

/// Parses a parenthesized `(red, green, blue)` expression tuple in the local
/// RGB dialect, which additionally supports `red(dx, dy)`, `green(dx, dy)`,
/// and `blue(dx, dy)` source-image sampling functions.
/// @throws std::invalid_argument for a missing argument or invalid tuple.
[[nodiscard]] RgbFormula parse_local_rgb_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] RgbFormula parse_local_rgb_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

/// Parses a parenthesized `(source_x, source_y)` expression pair.
/// @throws std::invalid_argument for a missing argument or invalid pair.
[[nodiscard]] WarpFormula parse_warp_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] WarpFormula parse_warp_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

/// Parses a parenthesized `(horizontal, vertical)` vector-field equation.
/// @throws std::invalid_argument for a missing argument or invalid pair.
[[nodiscard]] VectorFormula parse_vector_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] VectorFormula parse_vector_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

/// Parses a parenthesized `(source_x, source_y)` expression pair in the local
/// dialect, which additionally supports source-image sampling functions.
/// @throws std::invalid_argument for a missing argument or invalid pair.
[[nodiscard]] WarpFormula parse_local_warp_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] WarpFormula parse_local_warp_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

/// Parses an expression in the saturation dialect, where `S` replaces the RGB
/// channel variables.
/// @throws std::invalid_argument for a missing argument or invalid expression.
[[nodiscard]] Formula parse_saturation_formula(
    const std::vector<std::string> &arguments
);
[[nodiscard]] Formula parse_saturation_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
);

#endif //MAGRITTE_FORMULA_PARSE_H
