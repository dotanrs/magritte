#include "pixlie/processors/utils/formula_apply.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {

double evaluate(
    const FormulaNode& node,
    const Pixel& pixel,
    double saturation = 0.0
) {
    switch (node.kind) {
        case FormulaNodeKind::number:
            return node.number;
        case FormulaNodeKind::red:
            return pixel.red;
        case FormulaNodeKind::green:
            return pixel.green;
        case FormulaNodeKind::blue:
            return pixel.blue;
        case FormulaNodeKind::saturation:
            return saturation;
        case FormulaNodeKind::add:
            return evaluate(*node.left, pixel, saturation) +
                evaluate(*node.right, pixel, saturation);
        case FormulaNodeKind::subtract:
            return evaluate(*node.left, pixel, saturation) -
                evaluate(*node.right, pixel, saturation);
        case FormulaNodeKind::multiply:
            return evaluate(*node.left, pixel, saturation) *
                evaluate(*node.right, pixel, saturation);
        case FormulaNodeKind::divide: {
            const double numerator = evaluate(*node.left, pixel, saturation);
            const double denominator = evaluate(*node.right, pixel, saturation);
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
            return -evaluate(*node.left, pixel, saturation);
    }
    throw std::logic_error("unknown formula node");
}

double formula_value(double value) {
    if (std::isnan(value)) {
        value = 0.0;
    }
    return std::clamp(value, 0.0, 255.0);
}

std::uint8_t channel_value(double value) {
    return static_cast<std::uint8_t>(std::lround(formula_value(value)));
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

struct Hsl {
    double hue;
    double saturation;
    double lightness;
};

Hsl to_hsl(const Pixel& pixel) {
    const double red = pixel.red / 255.0;
    const double green = pixel.green / 255.0;
    const double blue = pixel.blue / 255.0;
    const double maximum = std::max({red, green, blue});
    const double minimum = std::min({red, green, blue});
    const double delta = maximum - minimum;
    const double lightness = (maximum + minimum) / 2.0;

    if (delta == 0.0) {
        return Hsl{.hue = 0.0, .saturation = 0.0, .lightness = lightness};
    }

    double hue;
    if (maximum == red) {
        hue = std::fmod((green - blue) / delta, 6.0);
    } else if (maximum == green) {
        hue = ((blue - red) / delta) + 2.0;
    } else {
        hue = ((red - green) / delta) + 4.0;
    }
    hue /= 6.0;
    if (hue < 0.0) {
        hue += 1.0;
    }

    return Hsl{
        .hue = hue,
        .saturation = delta / (1.0 - std::abs(2.0 * lightness - 1.0)),
        .lightness = lightness,
    };
}

void set_hsl(Pixel& pixel, const Hsl& hsl) {
    const double chroma =
        (1.0 - std::abs(2.0 * hsl.lightness - 1.0)) * hsl.saturation;
    const double hue_section = hsl.hue * 6.0;
    const double second =
        chroma * (1.0 - std::abs(std::fmod(hue_section, 2.0) - 1.0));

    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    if (hue_section < 1.0) {
        red = chroma;
        green = second;
    } else if (hue_section < 2.0) {
        red = second;
        green = chroma;
    } else if (hue_section < 3.0) {
        green = chroma;
        blue = second;
    } else if (hue_section < 4.0) {
        green = second;
        blue = chroma;
    } else if (hue_section < 5.0) {
        red = second;
        blue = chroma;
    } else {
        red = chroma;
        blue = second;
    }

    const double match = hsl.lightness - chroma / 2.0;
    pixel.red = channel_value((red + match) * 255.0);
    pixel.green = channel_value((green + match) * 255.0);
    pixel.blue = channel_value((blue + match) * 255.0);
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

FileData apply_saturation_formula(
    FileData data,
    const FormulaNode& formula
) {
    for (Pixel& pixel : data.pixels) {
        Hsl hsl = to_hsl(pixel);
        hsl.saturation =
            formula_value(evaluate(formula, pixel, hsl.saturation * 255.0)) /
            255.0;
        set_hsl(pixel, hsl);
    }
    return data;
}
