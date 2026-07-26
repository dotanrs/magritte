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
    add,
    subtract,
    multiply,
    divide,
    negate,
};

struct FormulaNode {
    FormulaNodeKind kind;
    double number = 0.0;
    std::unique_ptr<FormulaNode> left;
    std::unique_ptr<FormulaNode> right;
};

using Formula = std::unique_ptr<FormulaNode>;

[[nodiscard]] Formula parse_formula(
    const std::vector<std::string>& arguments
);

[[nodiscard]] Formula parse_saturation_formula(
    const std::vector<std::string>& arguments
);

#endif //PIXLIE_FORMULA_PARSE_H
