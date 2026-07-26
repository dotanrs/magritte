#include "pixlie/processors/utils/formula_apply.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {

double evaluate(const FormulaNode& node, const Pixel& pixel) {
    switch (node.kind) {
        case FormulaNodeKind::number:
            return node.number;
        case FormulaNodeKind::red:
            return pixel.red;
        case FormulaNodeKind::green:
            return pixel.green;
        case FormulaNodeKind::blue:
            return pixel.blue;
        case FormulaNodeKind::add:
            return evaluate(*node.left, pixel) + evaluate(*node.right, pixel);
        case FormulaNodeKind::subtract:
            return evaluate(*node.left, pixel) - evaluate(*node.right, pixel);
        case FormulaNodeKind::multiply:
            return evaluate(*node.left, pixel) * evaluate(*node.right, pixel);
        case FormulaNodeKind::divide: {
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
        case FormulaNodeKind::negate:
            return -evaluate(*node.left, pixel);
    }
    throw std::logic_error("unknown formula node");
}

std::uint8_t channel_value(double value) {
    if (std::isnan(value)) {
        value = 0.0;
    }
    value = std::clamp(value, 0.0, 255.0);
    return static_cast<std::uint8_t>(std::lround(value));
}

void set_channel(Pixel& pixel, ColorChannel channel, std::uint8_t value) {
    switch (channel) {
        case ColorChannel::red:
            pixel.red = value;
            return;
        case ColorChannel::green:
            pixel.green = value;
            return;
        case ColorChannel::blue:
            pixel.blue = value;
            return;
    }
    throw std::logic_error("unknown color channel");
}

} // namespace

FileData apply_formula(
    FileData data,
    const FormulaNode& formula,
    ColorChannel channel
) {
    for (Pixel& pixel : data.pixels) {
        set_channel(pixel, channel, channel_value(evaluate(formula, pixel)));
    }
    return data;
}
