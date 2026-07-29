#include "magritte/processors/utils/formula_apply.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "magritte/processors/utils/image_sample.h"

namespace {
    struct FormulaContext {
        const FileData &data;
        double red;
        double green;
        double blue;
        double x;
        double y;
        double width;
        double height;
        double polar_origin_x;
        double polar_origin_y;
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
                return context.red;
            case FormulaNodeKind::green:
                return context.green;
            case FormulaNodeKind::blue:
                return context.blue;
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
                const double offset_x = context.x - context.polar_origin_x;
                const double offset_y = context.y - context.polar_origin_y;
                return std::hypot(offset_x, offset_y);
            }
            case FormulaNodeKind::angle:
                return std::atan2(
                    context.y - context.polar_origin_y,
                    context.x - context.polar_origin_x
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
            case FormulaNodeKind::sample_red:
            case FormulaNodeKind::sample_green:
            case FormulaNodeKind::sample_blue: {
                const BilinearSample sampled = sample_bilinear_values(
                    context.data,
                    context.x + evaluate(*node.left, context),
                    context.y + evaluate(*node.right, context)
                );
                if (node.kind == FormulaNodeKind::sample_red) {
                    return sampled.red;
                }
                if (node.kind == FormulaNodeKind::sample_green) {
                    return sampled.green;
                }
                return sampled.blue;
            }
        }
        throw std::logic_error("unknown formula node");
    }

    FormulaContext make_context(
        const FileData &data,
        const Pixel &pixel,
        std::size_t x,
        std::size_t y,
        double saturation = 0.0,
        std::optional<FormulaPolarOrigin> polar_origin = std::nullopt
    ) {
        const double origin_x = polar_origin
            ? polar_origin->x * (data.width - 1.0)
            : (data.width - 1.0) / 2.0;
        const double origin_y = polar_origin
            ? polar_origin->y * (data.height - 1.0)
            : (data.height - 1.0) / 2.0;
        return FormulaContext{
            .data = data,
            .red = static_cast<double>(pixel.red),
            .green = static_cast<double>(pixel.green),
            .blue = static_cast<double>(pixel.blue),
            .x = static_cast<double>(x),
            .y = static_cast<double>(y),
            .width = static_cast<double>(data.width),
            .height = static_cast<double>(data.height),
            .polar_origin_x = origin_x,
            .polar_origin_y = origin_y,
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

double evaluate_formula_at(
    const FormulaNode &formula,
    const FileData &data,
    double x,
    double y
) {
    const BilinearSample sampled = sample_bilinear_values(data, x, y);
    const FormulaContext context{
        .data = data,
        .red = sampled.red,
        .green = sampled.green,
        .blue = sampled.blue,
        .x = x,
        .y = y,
        .width = static_cast<double>(data.width),
        .height = static_cast<double>(data.height),
        .polar_origin_x = (data.width - 1.0) / 2.0,
        .polar_origin_y = (data.height - 1.0) / 2.0,
    };
    return evaluate(formula, context);
}

FileData apply_rgb_formula(
    FileData data,
    const RgbFormula &formula
) {
    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            Pixel &pixel = data.pixels[y * data.width + x];
            const FormulaContext context = make_context(
                data,
                pixel,
                x,
                y,
                0.0,
                formula.polar_origin
            );

            std::array<std::uint8_t, 3> values{};
            for (std::size_t index = 0;
                 index < formula.expressions.size();
                 ++index) {
                values[index] = channel_value(
                    evaluate(*formula.expressions[index], context)
                );
            }
            for (std::size_t index = 0;
                 index < formula.channels.size();
                 ++index) {
                set_channel(pixel, formula.channels[index], values[index]);
            }
        }
    }
    return data;
}

FileData apply_local_rgb_formula(
    FileData data,
    const RgbFormula &formula
) {
    if (data.width == 0 || data.height == 0) {
        return data;
    }

    FileData result{
        .width = data.width,
        .height = data.height,
        .pixels = std::vector<Pixel>(data.pixels.size()),
    };

    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            const std::size_t index = y * data.width + x;
            const Pixel &source_pixel = data.pixels[index];
            const FormulaContext context =
                    make_context(data, source_pixel, x, y);

            Pixel output_pixel = source_pixel;
            for (std::size_t expression_index = 0;
                 expression_index < formula.channels.size();
                 ++expression_index) {
                set_channel(
                    output_pixel,
                    formula.channels[expression_index],
                    channel_value(
                        evaluate(
                            *formula.expressions[expression_index],
                            context
                        )
                    )
                );
            }
            result.pixels[index] = output_pixel;
        }
    }
    return result;
}

FileData apply_warp_formula(
    FileData data,
    const WarpFormula &formula
) {
    if (data.width == 0 || data.height == 0) {
        return data;
    }

    FileData result{
        .width = data.width,
        .height = data.height,
        .pixels = std::vector<Pixel>(data.pixels.size()),
    };

    for (std::size_t y = 0; y < data.height; ++y) {
        for (std::size_t x = 0; x < data.width; ++x) {
            const std::size_t index = y * data.width + x;
            const FormulaContext context =
                    make_context(data, data.pixels[index], x, y);
            const double source_x = evaluate(*formula.source_x, context);
            const double source_y = evaluate(*formula.source_y, context);
            result.pixels[index] =
                    sample_bilinear(data, source_x, source_y);
        }
    }
    return result;
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
