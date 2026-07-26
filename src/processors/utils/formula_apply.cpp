#include "pixlie/processors/utils/formula_apply.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {
    struct FormulaContext {
        const Pixel &pixel;
        double x;
        double y;
        double width;
        double height;
        double saturation = 0.0;
    };

    double normalized_coordinate(double coordinate, double extent) {
        if (extent <= 1.0) {
            return 0.0;
        }
        return 2.0 * coordinate / (extent - 1.0) - 1.0;
    }

    double evaluate(
        const FormulaNode &node,
        const FormulaContext &context
    ) {
        switch (node.kind) {
            case FormulaNodeKind::number:
                return node.number;
            case FormulaNodeKind::red:
                return context.pixel.red;
            case FormulaNodeKind::green:
                return context.pixel.green;
            case FormulaNodeKind::blue:
                return context.pixel.blue;
            case FormulaNodeKind::saturation:
                return context.saturation;
            case FormulaNodeKind::x:
                return context.x;
            case FormulaNodeKind::y:
                return context.y;
            case FormulaNodeKind::width:
                return context.width;
            case FormulaNodeKind::height:
                return context.height;
            case FormulaNodeKind::normalized_x:
                return normalized_coordinate(context.x, context.width);
            case FormulaNodeKind::normalized_y:
                return normalized_coordinate(context.y, context.height);
            case FormulaNodeKind::distance: {
                const double offset_x =
                        context.x - (context.width - 1.0) / 2.0;
                const double offset_y =
                        context.y - (context.height - 1.0) / 2.0;
                return std::hypot(offset_x, offset_y);
            }
            case FormulaNodeKind::angle:
                return std::atan2(
                    context.y - (context.height - 1.0) / 2.0,
                    context.x - (context.width - 1.0) / 2.0
                );
            case FormulaNodeKind::add:
                return evaluate(*node.left, context) +
                       evaluate(*node.right, context);
            case FormulaNodeKind::subtract:
                return evaluate(*node.left, context) -
                       evaluate(*node.right, context);
            case FormulaNodeKind::multiply:
                return evaluate(*node.left, context) *
                       evaluate(*node.right, context);
            case FormulaNodeKind::divide: {
                const double numerator = evaluate(*node.left, context);
                const double denominator = evaluate(*node.right, context);
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
                return -evaluate(*node.left, context);
            case FormulaNodeKind::sine:
                return std::sin(evaluate(*node.left, context));
            case FormulaNodeKind::cosine:
                return std::cos(evaluate(*node.left, context));
            case FormulaNodeKind::tangent:
                return std::tan(evaluate(*node.left, context));
            case FormulaNodeKind::arc_tangent_2:
                return std::atan2(
                    evaluate(*node.left, context),
                    evaluate(*node.right, context)
                );
            case FormulaNodeKind::square_root:
                return std::sqrt(evaluate(*node.left, context));
            case FormulaNodeKind::power:
                return std::pow(
                    evaluate(*node.left, context),
                    evaluate(*node.right, context)
                );
            case FormulaNodeKind::modulo:
                return std::fmod(
                    evaluate(*node.left, context),
                    evaluate(*node.right, context)
                );
            case FormulaNodeKind::absolute:
                return std::abs(evaluate(*node.left, context));
            case FormulaNodeKind::minimum:
                return std::min(
                    evaluate(*node.left, context),
                    evaluate(*node.right, context)
                );
            case FormulaNodeKind::maximum:
                return std::max(
                    evaluate(*node.left, context),
                    evaluate(*node.right, context)
                );
            case FormulaNodeKind::clamp: {
                const double value = evaluate(*node.left, context);
                const double first_bound = evaluate(*node.right, context);
                const double second_bound = evaluate(*node.third, context);
                return std::clamp(
                    value,
                    std::min(first_bound, second_bound),
                    std::max(first_bound, second_bound)
                );
            }
            case FormulaNodeKind::floor:
                return std::floor(evaluate(*node.left, context));
            case FormulaNodeKind::ceiling:
                return std::ceil(evaluate(*node.left, context));
            case FormulaNodeKind::round:
                return std::round(evaluate(*node.left, context));
            case FormulaNodeKind::exponential:
                return std::exp(evaluate(*node.left, context));
            case FormulaNodeKind::logarithm:
                return std::log(evaluate(*node.left, context));
        }
        throw std::logic_error("unknown formula node");
    }

    FormulaContext make_context(
        const FileData &data,
        const Pixel &pixel,
        std::size_t x,
        std::size_t y,
        double saturation = 0.0
    ) {
        return FormulaContext{
            .pixel = pixel,
            .x = static_cast<double>(x),
            .y = static_cast<double>(y),
            .width = static_cast<double>(data.width),
            .height = static_cast<double>(data.height),
            .saturation = saturation,
        };
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

    void set_channel(Pixel &pixel, ColorChannel channel, std::uint8_t value) {
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

    Hsl to_hsl(const Pixel &pixel) {
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

    void set_hsl(Pixel &pixel, const Hsl &hsl) {
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
    const FormulaNode &formula,
    ColorChannel channel
) {
    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            Pixel &pixel = data.pixels[y * data.width + x];
            set_channel(
                pixel,
                channel,
                channel_value(evaluate(formula, make_context(data, pixel, x, y)))
            );
        }
    }
    return data;
}

FileData apply_saturation_formula(
    FileData data,
    const FormulaNode &formula
) {
    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            Pixel &pixel = data.pixels[y * data.width + x];
            Hsl hsl = to_hsl(pixel);
            hsl.saturation = formula_value(
                evaluate(
                    formula,
                    make_context(data, pixel, x, y, hsl.saturation * 255.0)
                )
            ) / 255.0;
            set_hsl(pixel, hsl);
        }
    }
    return data;
}
