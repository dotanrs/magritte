#include "pixlie/processors/red_formula.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class NodeKind {
    number,
    red,
    green,
    blue,
    add,
    subtract,
    multiply,
    divide,
    negate,
};

struct Node {
    NodeKind kind;
    double number = 0.0;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

class FormulaParser {
public:
    explicit FormulaParser(std::string_view formula) : formula_(formula) {}

    [[nodiscard]] std::unique_ptr<Node> parse() {
        if (formula_.empty()) {
            throw std::invalid_argument("red formula cannot be empty");
        }

        std::unique_ptr<Node> result = parse_expression();
        skip_whitespace();
        if (position_ != formula_.size()) {
            fail("unexpected character");
        }
        return result;
    }

private:
    [[nodiscard]] std::unique_ptr<Node> parse_expression() {
        std::unique_ptr<Node> left = parse_term();

        while (true) {
            skip_whitespace();
            if (consume('+')) {
                left = binary(NodeKind::add, std::move(left), parse_term());
            } else if (consume('-')) {
                left = binary(NodeKind::subtract, std::move(left), parse_term());
            } else {
                return left;
            }
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parse_term() {
        std::unique_ptr<Node> left = parse_factor();

        while (true) {
            skip_whitespace();
            if (consume('*')) {
                left = binary(NodeKind::multiply, std::move(left), parse_factor());
            } else if (consume('/')) {
                left = binary(NodeKind::divide, std::move(left), parse_factor());
            } else {
                return left;
            }
        }
    }

    [[nodiscard]] std::unique_ptr<Node> parse_factor() {
        skip_whitespace();

        if (consume('+')) {
            return parse_factor();
        }
        if (consume('-')) {
            auto node = std::make_unique<Node>();
            node->kind = NodeKind::negate;
            node->left = parse_factor();
            return node;
        }
        if (consume('(')) {
            std::unique_ptr<Node> result = parse_expression();
            skip_whitespace();
            if (!consume(')')) {
                fail("expected ')'");
            }
            return result;
        }
        if (position_ >= formula_.size()) {
            fail("expected a number, variable, or '('");
        }

        const char token = formula_[position_];
        if (std::isalpha(static_cast<unsigned char>(token)) != 0) {
            ++position_;
            auto node = std::make_unique<Node>();
            switch (static_cast<char>(
                std::tolower(static_cast<unsigned char>(token))
            )) {
                case 'r':
                    node->kind = NodeKind::red;
                    break;
                case 'g':
                    node->kind = NodeKind::green;
                    break;
                case 'b':
                    node->kind = NodeKind::blue;
                    break;
                default:
                    fail("only R, G, and B variables are supported");
            }
            return node;
        }

        return parse_number();
    }

    [[nodiscard]] std::unique_ptr<Node> parse_number() {
        const std::size_t start = position_;
        bool has_digit = false;
        bool has_decimal_point = false;

        while (position_ < formula_.size()) {
            const char value = formula_[position_];
            if (std::isdigit(static_cast<unsigned char>(value)) != 0) {
                has_digit = true;
                ++position_;
            } else if (value == '.' && !has_decimal_point) {
                has_decimal_point = true;
                ++position_;
            } else {
                break;
            }
        }

        if (!has_digit) {
            fail("expected a number, variable, or '('");
        }

        const std::string number_text(formula_.substr(start, position_ - start));
        char* end = nullptr;
        const double value = std::strtod(number_text.c_str(), &end);
        if (end != number_text.c_str() + number_text.size() ||
            !std::isfinite(value)) {
            fail("invalid number");
        }

        auto node = std::make_unique<Node>();
        node->kind = NodeKind::number;
        node->number = value;
        return node;
    }

    [[nodiscard]] static std::unique_ptr<Node> binary(
        NodeKind kind,
        std::unique_ptr<Node> left,
        std::unique_ptr<Node> right
    ) {
        auto node = std::make_unique<Node>();
        node->kind = kind;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }

    void skip_whitespace() {
        while (position_ < formula_.size() &&
               std::isspace(static_cast<unsigned char>(formula_[position_])) != 0) {
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ < formula_.size() && formula_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    [[noreturn]] void fail(std::string_view message) const {
        throw std::invalid_argument(
            std::string(message) + " at position " + std::to_string(position_ + 1)
        );
    }

    std::string_view formula_;
    std::size_t position_ = 0;
};

double evaluate(const Node& node, const Pixel& pixel) {
    switch (node.kind) {
        case NodeKind::number:
            return node.number;
        case NodeKind::red:
            return pixel.red;
        case NodeKind::green:
            return pixel.green;
        case NodeKind::blue:
            return pixel.blue;
        case NodeKind::add:
            return evaluate(*node.left, pixel) + evaluate(*node.right, pixel);
        case NodeKind::subtract:
            return evaluate(*node.left, pixel) - evaluate(*node.right, pixel);
        case NodeKind::multiply:
            return evaluate(*node.left, pixel) * evaluate(*node.right, pixel);
        case NodeKind::divide: {
            const double numerator = evaluate(*node.left, pixel);
            const double denominator = evaluate(*node.right, pixel);
            if (denominator == 0.0) {
                if (numerator == 0.0) {
                    return 0.0;
                }
                return std::copysign(
                    std::numeric_limits<double>::infinity(),
                    numerator
                );
            }
            return numerator / denominator;
        }
        case NodeKind::negate:
            return -evaluate(*node.left, pixel);
    }
    throw std::logic_error("unknown formula node");
}

std::unique_ptr<Node> parse_formula(const std::vector<std::string>& arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument("red processor expects one formula");
    }
    return FormulaParser(arguments.front()).parse();
}

class RedFormulaProcessor final : public ImageProcessor {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "red formula";
    }

    void validate(const std::vector<std::string>& arguments) const override {
        static_cast<void>(parse_formula(arguments));
    }

    [[nodiscard]] FileData apply(
        FileData data,
        const std::vector<std::string>& arguments
    ) const override {
        const std::unique_ptr<Node> formula = parse_formula(arguments);

        for (Pixel& pixel : data.pixels) {
            double red = evaluate(*formula, pixel);
            if (std::isnan(red)) {
                red = 0.0;
            }
            red = std::clamp(red, 0.0, 255.0);
            pixel.red = static_cast<std::uint8_t>(std::lround(red));
        }

        return data;
    }
};

} // namespace

const ImageProcessor& red_formula_processor() {
    static const RedFormulaProcessor processor;
    return processor;
}
