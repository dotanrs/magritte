#include "magritte/steps/utils/formula_parse.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
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
        {"red", FormulaNodeKind::sample_red, 2},
        {"green", FormulaNodeKind::sample_green, 2},
        {"blue", FormulaNodeKind::sample_blue, 2},
    };

    class FormulaParser {
    public:
        explicit FormulaParser(
            const std::string_view formula,
            const bool allow_saturation_variables = false,
            const bool allow_other_cell_references = false,
            const MacroMap *macros = nullptr,
            std::vector<std::string> *macros_currently_unpacking = nullptr
        ) : formula_(formula),
            allow_saturation_variables_(allow_saturation_variables),
            allow_other_cell_references_(allow_other_cell_references),
            macros_(macros),
            macros_currently_unpacking_(macros_currently_unpacking) {
            if (macros_currently_unpacking_ == nullptr) {
                macros_currently_unpacking_ = &macro_stack_storage_;
            }
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

        [[nodiscard]] RgbFormula parse_rgb(
            std::vector<ColorChannel> channels
        ) {
            if (formula_.empty()) {
                throw std::invalid_argument("RGB formula cannot be empty");
            }

            RgbFormula result{
                .channels = std::move(channels),
                .expressions = {},
            };
            result.expressions.reserve(result.channels.size());

            if (result.channels.size() == 1) {
                result.expressions.push_back(parse_expression());
                skip_whitespace();
                if (position_ != formula_.size()) {
                    fail("unexpected character after RGB formula");
                }
                return result;
            }

            skip_whitespace();
            if (!consume('(')) {
                fail("RGB formula must be a parenthesized tuple");
            }

            for (std::size_t index = 0;
                 index < result.channels.size();
                 ++index) {
                skip_whitespace();
                if (position_ < formula_.size() &&
                    formula_[position_] == ')') {
                    fail(
                        "RGB formula value count must match the target "
                        "channel count"
                    );
                }
                result.expressions.push_back(parse_expression());
                if (index + 1 < result.channels.size()) {
                    skip_whitespace();
                    if (position_ < formula_.size() &&
                        formula_[position_] == ')') {
                        fail(
                            "RGB formula value count must match the target "
                            "channel count"
                        );
                    }
                    expect_tuple_separator();
                }
            }

            skip_whitespace();
            if (!consume(')')) {
                fail(
                    "RGB formula value count must match the target channel count"
                );
            }
            skip_whitespace();
            if (position_ != formula_.size()) {
                fail("unexpected character after RGB formula");
            }
            return result;
        }

        [[nodiscard]] WarpFormula parse_warp() {
            if (formula_.empty()) {
                throw std::invalid_argument("warp formula cannot be empty");
            }

            skip_whitespace();
            if (!consume('(')) {
                fail("warp formula must be a parenthesized coordinate pair");
            }

            WarpFormula result;
            result.source_x = parse_expression();
            expect_tuple_separator("warp coordinates");
            result.source_y = parse_expression();

            skip_whitespace();
            if (!consume(')')) {
                fail("expected ')' after the source y expression");
            }
            skip_whitespace();
            if (position_ != formula_.size()) {
                fail("unexpected character after warp formula");
            }
            return result;
        }

        [[nodiscard]] VectorFormula parse_vector() {
            if (formula_.empty()) {
                throw std::invalid_argument(
                    "flow field equation cannot be empty"
                );
            }

            skip_whitespace();
            if (!consume('(')) {
                fail(
                    "flow field equation must be a parenthesized vector pair"
                );
            }

            VectorFormula result;
            result.x = parse_expression();
            expect_tuple_separator("flow field components");
            result.y = parse_expression();

            skip_whitespace();
            if (!consume(')')) {
                fail("expected ')' after the vertical field expression");
            }
            skip_whitespace();
            if (position_ != formula_.size()) {
                fail("unexpected character after flow field equation");
            }
            return result;
        }

    private:
        void expect_tuple_separator(
            std::string_view tuple_name = "RGB expressions"
        ) {
            skip_whitespace();
            if (!consume(',')) {
                fail("expected ',' between " + std::string(tuple_name));
            }
        }

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
            if (!allow_other_cell_references_ &&
                (definition->kind == FormulaNodeKind::sample_red ||
                 definition->kind == FormulaNodeKind::sample_green ||
                 definition->kind == FormulaNodeKind::sample_blue)) {
                fail(
                    "function '" + name +
                    "' is only available in local-rgb and local-warp formulas"
                );
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
            if (name.starts_with("macro_")) {
                return parse_macro(name);
            }

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
            } else if (allow_saturation_variables_ && name == "s") {
                node->kind = FormulaNodeKind::saturation;
            } else if (!allow_saturation_variables_ && name == "r") {
                node->kind = FormulaNodeKind::red;
            } else if (!allow_saturation_variables_ && name == "g") {
                node->kind = FormulaNodeKind::green;
            } else if (!allow_saturation_variables_ && name == "b") {
                node->kind = FormulaNodeKind::blue;
            } else {
                fail(
                    allow_saturation_variables_
                        ? "unknown identifier; use S, coordinates, or a constant"
                        : "unknown identifier; use R, G, B, coordinates, or a constant"
                );
            }
            return node;
        }

        [[nodiscard]] Formula parse_macro(const std::string &name) {
            if (macros_ == nullptr) {
                // in the resolution phase, macros are not available but assumed correct.
                auto placeholder = std::make_unique<FormulaNode>();
                placeholder->kind = FormulaNodeKind::number;
                return placeholder;
            }

            const auto definition = macros_->find(name);
            if (definition == macros_->end()) {
                fail("unknown macro '" + name + "'");
            }

            // Check for cyclic macro usage
            if (std::find(
                    macros_currently_unpacking_->begin(),
                    macros_currently_unpacking_->end(),
                    name
                ) != macros_currently_unpacking_->end()) {
                fail("cyclic macro reference involving '" + name + "'");
            }

            macros_currently_unpacking_->push_back(name);
            try {
                // Parse the macro definition formula (now that we know it's not cyclic)
                Formula expansion = FormulaParser(
                    definition->second,
                    allow_saturation_variables_,
                    allow_other_cell_references_,
                    macros_,
                    macros_currently_unpacking_
                ).parse();
                macros_currently_unpacking_->pop_back();
                return expansion;
            } catch (...) {
                macros_currently_unpacking_->pop_back();
                throw;
            }
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
        bool allow_saturation_variables_;
        bool allow_other_cell_references_;
        const MacroMap *macros_;
        std::vector<std::string> macro_stack_storage_;
        // Saves the macros currently being expanded (they might reference each other)
        std::vector<std::string> *macros_currently_unpacking_;
    };

    std::vector<ColorChannel> parse_rgb_target(std::string_view target) {
        if (target.empty()) {
            throw std::invalid_argument("RGB formula target cannot be empty");
        }

        std::vector<ColorChannel> channels;
        channels.reserve(target.size());
        bool has_red = false;
        bool has_green = false;
        bool has_blue = false;

        for (const char value: target) {
            ColorChannel channel;
            bool *already_present;
            switch (value) {
                case 'r':
                    channel = ColorChannel::red;
                    already_present = &has_red;
                    break;
                case 'g':
                    channel = ColorChannel::green;
                    already_present = &has_green;
                    break;
                case 'b':
                    channel = ColorChannel::blue;
                    already_present = &has_blue;
                    break;
                default:
                    throw std::invalid_argument(
                        "RGB formula target may only contain r, g, and b"
                    );
            }

            if (*already_present) {
                throw std::invalid_argument(
                    "RGB formula target cannot repeat a channel"
                );
            }
            *already_present = true;
            channels.push_back(channel);
        }
        return channels;
    }

    double parse_rgb_offset_number(
        const std::string &text,
        std::string_view name
    ) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "RGB formula offset " + std::string(name) +
                " must be a finite number"
            );
        }
        return value;
    }

    FormulaPolarOrigin parse_rgb_offset(
        const std::vector<std::string> &arguments
    ) {
        FormulaPolarOrigin origin{
            .x_percent = parse_rgb_offset_number(arguments[2], "x"),
            .y_percent = parse_rgb_offset_number(arguments[3], "y"),
            .radius_percent = arguments.size() == 5
                ? std::optional<double>(
                    parse_rgb_offset_number(arguments[4], "radius")
                )
                : std::nullopt,
        };
        if (origin.x_percent < 0.0 || origin.x_percent > 100.0 ||
            origin.y_percent < 0.0 || origin.y_percent > 100.0) {
            throw std::invalid_argument(
                "RGB formula offset x and y must be percentages from 0 to 100"
            );
        }
        if (origin.radius_percent && *origin.radius_percent <= 0.0) {
            throw std::invalid_argument(
                "RGB formula offset radius must be greater than 0"
            );
        }
        return origin;
    }
} // namespace

RgbFormula parse_rgb_formula(const std::vector<std::string> &arguments) {
    if (arguments.size() == 1) {
        return FormulaParser(arguments.front()).parse_rgb(
            parse_rgb_target("rgb")
        );
    }
    if (arguments.size() == 2) {
        return FormulaParser(arguments[1]).parse_rgb(
            parse_rgb_target(arguments[0])
        );
    }
    if (arguments.size() == 4 || arguments.size() == 5) {
        RgbFormula formula = FormulaParser(arguments[1]).parse_rgb(
            parse_rgb_target(arguments[0])
        );
        formula.polar_origin = parse_rgb_offset(arguments);
        return formula;
    }
    throw std::invalid_argument(
        "RGB formula step expects a target and formula, optionally "
        "followed by offset x, y, and radius"
    );
}

RgbFormula parse_rgb_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() == 1) {
        return FormulaParser(
            arguments.front(),
            false,
            false,
            &macros
        ).parse_rgb(parse_rgb_target("rgb"));
    }
    if (arguments.size() == 2 || arguments.size() == 4 ||
        arguments.size() == 5) {
        RgbFormula formula = FormulaParser(
            arguments[1],
            false,
            false,
            &macros
        ).parse_rgb(parse_rgb_target(arguments[0]));
        if (arguments.size() >= 4) {
            formula.polar_origin = parse_rgb_offset(arguments);
        }
        return formula;
    }
    throw std::invalid_argument(
        "RGB formula step expects a target and formula, optionally "
        "followed by offset x, y, and radius"
    );
}

RgbFormula parse_local_rgb_formula(
    const std::vector<std::string> &arguments
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "local RGB formula step expects one tuple"
        );
    }
    return FormulaParser(arguments.front(), false, true).parse_rgb(
        parse_rgb_target("rgb")
    );
}

RgbFormula parse_local_rgb_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "local RGB formula step expects one tuple"
        );
    }
    return FormulaParser(
        arguments.front(),
        false,
        true,
        &macros
    ).parse_rgb(parse_rgb_target("rgb"));
}

WarpFormula parse_warp_formula(const std::vector<std::string> &arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "warp formula step expects one coordinate pair"
        );
    }
    return FormulaParser(arguments.front()).parse_warp();
}

WarpFormula parse_warp_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "warp formula step expects one coordinate pair"
        );
    }
    return FormulaParser(
        arguments.front(),
        false,
        false,
        &macros
    ).parse_warp();
}

VectorFormula parse_vector_formula(
    const std::vector<std::string> &arguments
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "flow field step expects one vector pair"
        );
    }
    return FormulaParser(arguments.front()).parse_vector();
}

VectorFormula parse_vector_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "flow field step expects one vector pair"
        );
    }
    return FormulaParser(
        arguments.front(),
        false,
        false,
        &macros
    ).parse_vector();
}

WarpFormula parse_local_warp_formula(
    const std::vector<std::string> &arguments
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "local warp formula step expects one coordinate pair"
        );
    }
    return FormulaParser(arguments.front(), false, true).parse_warp();
}

WarpFormula parse_local_warp_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "local warp formula step expects one coordinate pair"
        );
    }
    return FormulaParser(
        arguments.front(),
        false,
        true,
        &macros
    ).parse_warp();
}

Formula parse_saturation_formula(const std::vector<std::string> &arguments) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "saturation formula step expects one formula"
        );
    }
    return FormulaParser(arguments.front(), true).parse();
}

Formula parse_saturation_formula(
    const std::vector<std::string> &arguments,
    const MacroMap &macros
) {
    if (arguments.size() != 1) {
        throw std::invalid_argument(
            "saturation formula step expects one formula"
        );
    }
    return FormulaParser(
        arguments.front(),
        true,
        false,
        &macros
    ).parse();
}
