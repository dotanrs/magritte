#include "pixlie/processors/utils/formula_parse.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    struct FunctionDefinition {
        std::string_view name;
        FormulaNodeKind kind;
        std::size_t arity;
    };

    constexpr FunctionDefinition functions[] = {
        {"sin", FormulaNodeKind::sine, 1},
        {"cos", FormulaNodeKind::cosine, 1},
        {"tan", FormulaNodeKind::tangent, 1},
        {"atan2", FormulaNodeKind::arc_tangent_2, 2},
        {"sqrt", FormulaNodeKind::square_root, 1},
        {"pow", FormulaNodeKind::power, 2},
        {"mod", FormulaNodeKind::modulo, 2},
        {"abs", FormulaNodeKind::absolute, 1},
        {"min", FormulaNodeKind::minimum, 2},
        {"max", FormulaNodeKind::maximum, 2},
        {"clamp", FormulaNodeKind::clamp, 3},
        {"floor", FormulaNodeKind::floor, 1},
        {"ceil", FormulaNodeKind::ceiling, 1},
        {"round", FormulaNodeKind::round, 1},
        {"exp", FormulaNodeKind::exponential, 1},
        {"log", FormulaNodeKind::logarithm, 1},
    };

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
                return parse_identifier();
            }

            return parse_number();
        }

        [[nodiscard]] Formula parse_identifier() {
            const std::size_t start = position_;
            while (position_ < formula_.size()) {
                const unsigned char value =
                        static_cast<unsigned char>(formula_[position_]);
                if (std::isalnum(value) == 0 && value != '_') {
                    break;
                }
                ++position_;
            }

            std::string identifier(formula_.substr(start, position_ - start));
            for (char &value: identifier) {
                value = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(value))
                );
            }

            skip_whitespace();
            if (consume('(')) {
                return parse_function(identifier);
            }
            return parse_variable_or_constant(identifier);
        }

        [[nodiscard]] Formula parse_function(const std::string &name) {
            const FunctionDefinition *definition = nullptr;
            for (const FunctionDefinition &candidate: functions) {
                if (candidate.name == name) {
                    definition = &candidate;
                    break;
                }
            }
            if (definition == nullptr) {
                fail("unknown function '" + name + "'");
            }

            std::vector<Formula> arguments;
            skip_whitespace();
            if (!consume(')')) {
                while (true) {
                    arguments.push_back(parse_expression());
                    skip_whitespace();
                    if (consume(')')) {
                        break;
                    }
                    if (!consume(',')) {
                        fail("expected ',' or ')'");
                    }
                }
            }

            if (arguments.size() != definition->arity) {
                fail(
                    "function '" + name + "' expects " +
                    std::to_string(definition->arity) + " argument" +
                    (definition->arity == 1 ? "" : "s")
                );
            }

            auto node = std::make_unique<FormulaNode>();
            node->kind = definition->kind;
            node->left = std::move(arguments[0]);
            if (arguments.size() >= 2) {
                node->right = std::move(arguments[1]);
            }
            if (arguments.size() >= 3) {
                node->third = std::move(arguments[2]);
            }
            return node;
        }

        [[nodiscard]] Formula parse_variable_or_constant(
            const std::string &name
        ) {
            auto node = std::make_unique<FormulaNode>();
            if (name == "pi") {
                node->kind = FormulaNodeKind::number;
                node->number = std::numbers::pi;
            } else if (name == "e") {
                node->kind = FormulaNodeKind::number;
                node->number = std::numbers::e;
            } else if (name == "x") {
                node->kind = FormulaNodeKind::x;
            } else if (name == "y") {
                node->kind = FormulaNodeKind::y;
            } else if (name == "w") {
                node->kind = FormulaNodeKind::width;
            } else if (name == "h") {
                node->kind = FormulaNodeKind::height;
            } else if (name == "u") {
                node->kind = FormulaNodeKind::normalized_x;
            } else if (name == "v") {
                node->kind = FormulaNodeKind::normalized_y;
            } else if (name == "d") {
                node->kind = FormulaNodeKind::distance;
            } else if (name == "a") {
                node->kind = FormulaNodeKind::angle;
            } else if (saturation_formula_ && name == "s") {
                node->kind = FormulaNodeKind::saturation;
            } else if (!saturation_formula_ && name == "r") {
                node->kind = FormulaNodeKind::red;
            } else if (!saturation_formula_ && name == "g") {
                node->kind = FormulaNodeKind::green;
            } else if (!saturation_formula_ && name == "b") {
                node->kind = FormulaNodeKind::blue;
            } else {
                fail(
                    saturation_formula_
                        ? "unknown identifier; use S, coordinates, or a constant"
                        : "unknown identifier; use R, G, B, coordinates, or a constant"
                );
            }
            return node;
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
