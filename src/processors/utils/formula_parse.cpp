#include "pixlie/processors/utils/formula_parse.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {
    class FormulaParser {
    public:
        explicit FormulaParser(
            std::string_view formula,
            bool saturation_formula = false
        ) : formula_(formula), saturation_formula_(saturation_formula) {
        }

        [[nodiscard]] Formula parse() {
            if (formula_.empty()) {
                throw std::invalid_argument("formula cannot be empty");
            }

            Formula result = parse_expression();
            skip_whitespace();
            if (position_ != formula_.size()) {
                fail("unexpected character");
            }
            return result;
        }

    private:
        [[nodiscard]] Formula parse_expression() {
            Formula left = parse_term();

            while (true) {
                skip_whitespace();
                if (consume('+')) {
                    left = binary(
                        FormulaNodeKind::add,
                        std::move(left),
                        parse_term()
                    );
                } else if (consume('-')) {
                    left = binary(
                        FormulaNodeKind::subtract,
                        std::move(left),
                        parse_term()
                    );
                } else {
                    return left;
                }
            }
        }

        [[nodiscard]] Formula parse_term() {
            Formula left = parse_factor();

            while (true) {
                skip_whitespace();
                if (consume('*')) {
                    left = binary(
                        FormulaNodeKind::multiply,
                        std::move(left),
                        parse_factor()
                    );
                } else if (consume('/')) {
                    left = binary(
                        FormulaNodeKind::divide,
                        std::move(left),
                        parse_factor()
                    );
                } else {
                    return left;
                }
            }
        }

        [[nodiscard]] Formula parse_factor() {
            skip_whitespace();

            if (consume('+')) {
                return parse_factor();
            }
            if (consume('-')) {
                auto node = std::make_unique<FormulaNode>();
                node->kind = FormulaNodeKind::negate;
                node->left = parse_factor();
                return node;
            }
            if (consume('(')) {
                Formula result = parse_expression();
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
                auto node = std::make_unique<FormulaNode>();
                const char variable = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(token))
                );
                if (saturation_formula_) {
                    if (variable != 's') {
                        fail("only the S variable is supported");
                    }
                    node->kind = FormulaNodeKind::saturation;
                    return node;
                }
                switch (variable) {
                    case 'r':
                        node->kind = FormulaNodeKind::red;
                        break;
                    case 'g':
                        node->kind = FormulaNodeKind::green;
                        break;
                    case 'b':
                        node->kind = FormulaNodeKind::blue;
                        break;
                    default:
                        fail("only R, G, and B variables are supported");
                }
                return node;
            }

            return parse_number();
        }

        [[nodiscard]] Formula parse_number() {
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
            char *end = nullptr;
            const double value = std::strtod(number_text.c_str(), &end);
            if (end != number_text.c_str() + number_text.size() ||
                !std::isfinite(value)) {
                fail("invalid number");
            }

            auto node = std::make_unique<FormulaNode>();
            node->kind = FormulaNodeKind::number;
            node->number = value;
            return node;
        }

        [[nodiscard]] static Formula binary(
            FormulaNodeKind kind,
            Formula left,
            Formula right
        ) {
            auto node = std::make_unique<FormulaNode>();
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
        bool saturation_formula_;
    };
} // namespace

Formula parse_formula(const std::vector<std::string> &arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument("formula processor expects one formula");
    }
    return FormulaParser(arguments.front()).parse();
}

Formula parse_saturation_formula(const std::vector<std::string> &arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "saturation formula processor expects one formula"
        );
    }
    return FormulaParser(arguments.front(), true).parse();
}
