// Processor: `flow-lines <spacing> <steps> <step-size> <width> <#RRGGBB>
// [opacity] = (<VX>, <VY>)`.
// Draws antialiased streamlines through a formula-defined vector field.
// `spacing` is the seed and minimum path separation in pixels; `steps` is the
// maximum integration steps in each direction; `step-size` is the distance
// traveled per step; `width` is the stroke width in pixels; `#RRGGBB` is the
// stroke color; optional `opacity` is from 0 to 1 and defaults to 1; `VX` and
// `VY` are formulas for the field's horizontal and vertical components.

#include "magritte/processors/flow_lines.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "magritte/processors/utils/argument_parse.h"
#include "magritte/processors/utils/formula_apply.h"
#include "magritte/processors/utils/formula_parse.h"

namespace {
    struct Point {
        double x;
        double y;
    };

    struct FlowLineArguments {
        double spacing;
        std::size_t steps;
        double step_size;
        double width;
        Pixel color;
        double opacity;
        VectorFormula field;
    };

    double parse_number(
        const std::string &text,
        std::string_view field,
        double minimum,
        double maximum
    ) {
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value) ||
            value < minimum || value > maximum) {
            throw std::invalid_argument(
                "flow-lines " + std::string(field) + " must be from " +
                std::to_string(minimum) + " to " +
                std::to_string(maximum)
            );
        }
        return value;
    }

    std::size_t parse_steps(const std::string &text) {
        std::size_t value = 0;
        const auto [end, error] = std::from_chars(
            text.data(),
            text.data() + text.size(),
            value
        );
        if (error != std::errc{} || end != text.data() + text.size() ||
            value == 0 || value > 10000) {
            throw std::invalid_argument(
                "flow-lines steps must be an integer from 1 to 10000"
            );
        }
        return value;
    }

    std::uint8_t parse_hex_byte(std::string_view value) {
        unsigned int result = 0;
        const auto [end, error] = std::from_chars(
            value.data(),
            value.data() + value.size(),
            result,
            16
        );
        if (error != std::errc{} || end != value.data() + value.size()) {
            throw std::invalid_argument(
                "flow-lines color must use #RRGGBB"
            );
        }
        return static_cast<std::uint8_t>(result);
    }

    Pixel parse_color(const std::string &text) {
        if (text.size() != 7 || text.front() != '#') {
            throw std::invalid_argument(
                "flow-lines color must use #RRGGBB"
            );
        }
        return Pixel{
            .red = parse_hex_byte(std::string_view(text).substr(1, 2)),
            .green = parse_hex_byte(std::string_view(text).substr(3, 2)),
            .blue = parse_hex_byte(std::string_view(text).substr(5, 2)),
            .alpha = 255,
        };
    }

    FlowLineArguments parse_flow_line_arguments(
        const std::vector<std::string> &arguments,
        const MacroMap *macros = nullptr
    ) {
        if (arguments.size() != 6 && arguments.size() != 7) {
            throw std::invalid_argument(
                "flow-lines expects spacing steps step-size width "
                "#RRGGBB [opacity] = (VX, VY)"
            );
        }
        const bool has_opacity = arguments.size() == 7;
        return FlowLineArguments{
            .spacing = parse_number(
                arguments[0],
                "spacing",
                1.0,
                10000.0
            ),
            .steps = parse_steps(arguments[1]),
            .step_size = parse_number(
                arguments[2],
                "step-size",
                0.01,
                100.0
            ),
            .width = parse_number(
                arguments[3],
                "width",
                0.1,
                100.0
            ),
            .color = parse_color(arguments[4]),
            .opacity = has_opacity
                ? parse_number(arguments[5], "opacity", 0.0, 1.0)
                : 1.0,
            .field = macros == nullptr
                ? parse_vector_formula({
                    arguments[has_opacity ? 6 : 5]
                })
                : parse_vector_formula(
                    {arguments[has_opacity ? 6 : 5]},
                    *macros
                ),
        };
    }

    std::optional<std::vector<std::string>> parse_command(
        std::string_view command
    ) {
        constexpr std::string_view keyword = "flow-lines";
        const std::string_view value =
            processor_argument_parse::trim(command);
        if (!value.starts_with(keyword) ||
            (value.size() > keyword.size() &&
             !std::isspace(
                 static_cast<unsigned char>(value[keyword.size()])
             ))) {
            return std::nullopt;
        }

        const std::size_t equals = value.find('=');
        if (equals == std::string_view::npos) {
            throw std::invalid_argument(
                "flow-lines expects '= (VX, VY)' after its style arguments"
            );
        }
        auto words = processor_argument_parse::split_words(
            processor_argument_parse::trim(value.substr(0, equals))
        );
        if (words.empty() || words.front() != keyword) {
            return std::nullopt;
        }
        words.erase(words.begin());
        words.emplace_back(
            processor_argument_parse::trim(value.substr(equals + 1))
        );
        return words;
    }

    class ProximityMap {
    public:
        ProximityMap(
            std::size_t width,
            std::size_t height,
            double minimum_distance
        ) : cell_size_(std::max(1.0, minimum_distance)),
            minimum_distance_squared_(minimum_distance * minimum_distance),
            columns_(std::max<std::size_t>(
                1,
                static_cast<std::size_t>(
                    std::ceil(static_cast<double>(width) / cell_size_)
                )
            )),
            rows_(std::max<std::size_t>(
                1,
                static_cast<std::size_t>(
                    std::ceil(static_cast<double>(height) / cell_size_)
                )
            )),
            cells_(columns_ * rows_) {
        }

        [[nodiscard]] bool near(const Point &point) const {
            const long cell_x = static_cast<long>(point.x / cell_size_);
            const long cell_y = static_cast<long>(point.y / cell_size_);
            for (long y = std::max(0L, cell_y - 1);
                 y <= std::min(static_cast<long>(rows_) - 1, cell_y + 1);
                 ++y) {
                for (long x = std::max(0L, cell_x - 1);
                     x <= std::min(
                         static_cast<long>(columns_) - 1,
                         cell_x + 1
                     );
                     ++x) {
                    for (const Point &existing:
                         cells_[static_cast<std::size_t>(y) * columns_ +
                                static_cast<std::size_t>(x)]) {
                        const double dx = existing.x - point.x;
                        const double dy = existing.y - point.y;
                        if (dx * dx + dy * dy <
                            minimum_distance_squared_) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        void add(const std::vector<Point> &path) {
            for (const Point &point: path) {
                const std::size_t x = std::min(
                    columns_ - 1,
                    static_cast<std::size_t>(point.x / cell_size_)
                );
                const std::size_t y = std::min(
                    rows_ - 1,
                    static_cast<std::size_t>(point.y / cell_size_)
                );
                cells_[y * columns_ + x].push_back(point);
            }
        }

    private:
        double cell_size_;
        double minimum_distance_squared_;
        std::size_t columns_;
        std::size_t rows_;
        std::vector<std::vector<Point>> cells_;
    };

    [[nodiscard]] bool in_bounds(const FileData &data, const Point &point) {
        return point.x >= 0.0 && point.y >= 0.0 &&
               point.x <= static_cast<double>(data.width - 1) &&
               point.y <= static_cast<double>(data.height - 1);
    }

    std::optional<Point> field_direction(
        const FileData &data,
        const VectorFormula &field,
        const Point &point,
        double direction
    ) {
        const double x = evaluate_formula_at(
            *field.x,
            data,
            point.x,
            point.y
        );
        const double y = evaluate_formula_at(
            *field.y,
            data,
            point.x,
            point.y
        );
        const double magnitude = std::hypot(x, y);
        if (!std::isfinite(magnitude) || magnitude < 1e-9) {
            return std::nullopt;
        }
        return Point{
            .x = direction * x / magnitude,
            .y = direction * y / magnitude,
        };
    }

    std::optional<Point> rk4_step(
        const FileData &data,
        const VectorFormula &field,
        const Point &point,
        double step_size,
        double direction
    ) {
        const auto k1 = field_direction(data, field, point, direction);
        if (!k1) {
            return std::nullopt;
        }
        const Point second{
            .x = point.x + step_size * k1->x / 2.0,
            .y = point.y + step_size * k1->y / 2.0,
        };
        if (!in_bounds(data, second)) {
            return std::nullopt;
        }
        const auto k2 = field_direction(data, field, second, direction);
        if (!k2) {
            return std::nullopt;
        }
        const Point third{
            .x = point.x + step_size * k2->x / 2.0,
            .y = point.y + step_size * k2->y / 2.0,
        };
        if (!in_bounds(data, third)) {
            return std::nullopt;
        }
        const auto k3 = field_direction(data, field, third, direction);
        if (!k3) {
            return std::nullopt;
        }
        const Point fourth{
            .x = point.x + step_size * k3->x,
            .y = point.y + step_size * k3->y,
        };
        if (!in_bounds(data, fourth)) {
            return std::nullopt;
        }
        const auto k4 = field_direction(data, field, fourth, direction);
        if (!k4) {
            return std::nullopt;
        }

        const Point result{
            .x = point.x + step_size *
                (k1->x + 2.0 * k2->x + 2.0 * k3->x + k4->x) / 6.0,
            .y = point.y + step_size *
                (k1->y + 2.0 * k2->y + 2.0 * k3->y + k4->y) / 6.0,
        };
        return in_bounds(data, result)
            ? std::optional<Point>(result)
            : std::nullopt;
    }

    std::vector<Point> trace_direction(
        const FileData &data,
        const FlowLineArguments &arguments,
        const ProximityMap &occupied,
        const Point &seed,
        double direction
    ) {
        std::vector<Point> path{seed};
        path.reserve(arguments.steps + 1);
        Point point = seed;
        for (std::size_t index = 0; index < arguments.steps; ++index) {
            const auto next = rk4_step(
                data,
                arguments.field,
                point,
                arguments.step_size,
                direction
            );
            if (!next || occupied.near(*next)) {
                break;
            }
            const double movement = std::hypot(
                next->x - point.x,
                next->y - point.y
            );
            if (!std::isfinite(movement) || movement < 1e-6) {
                break;
            }
            path.push_back(*next);
            point = *next;
        }
        return path;
    }

    std::vector<Point> trace_streamline(
        const FileData &data,
        const FlowLineArguments &arguments,
        const ProximityMap &occupied,
        const Point &seed
    ) {
        std::vector<Point> backward = trace_direction(
            data,
            arguments,
            occupied,
            seed,
            -1.0
        );
        std::vector<Point> forward = trace_direction(
            data,
            arguments,
            occupied,
            seed,
            1.0
        );
        std::reverse(backward.begin(), backward.end());
        backward.insert(
            backward.end(),
            std::next(forward.begin()),
            forward.end()
        );
        return backward;
    }

    double path_length(const std::vector<Point> &path) {
        double result = 0.0;
        for (std::size_t index = 1; index < path.size(); ++index) {
            result += std::hypot(
                path[index].x - path[index - 1].x,
                path[index].y - path[index - 1].y
            );
        }
        return result;
    }

    void rasterize_segment(
        std::vector<float> &coverage,
        std::size_t width,
        std::size_t height,
        const Point &first,
        const Point &second,
        double line_width
    ) {
        const double radius = line_width / 2.0;
        const double extent = radius + 1.0;
        const long minimum_x = std::max(
            0L,
            static_cast<long>(std::floor(
                std::min(first.x, second.x) - extent
            ))
        );
        const long maximum_x = std::min(
            static_cast<long>(width) - 1,
            static_cast<long>(std::ceil(
                std::max(first.x, second.x) + extent
            ))
        );
        const long minimum_y = std::max(
            0L,
            static_cast<long>(std::floor(
                std::min(first.y, second.y) - extent
            ))
        );
        const long maximum_y = std::min(
            static_cast<long>(height) - 1,
            static_cast<long>(std::ceil(
                std::max(first.y, second.y) + extent
            ))
        );
        const double segment_x = second.x - first.x;
        const double segment_y = second.y - first.y;
        const double length_squared =
            segment_x * segment_x + segment_y * segment_y;

        for (long y = minimum_y; y <= maximum_y; ++y) {
            for (long x = minimum_x; x <= maximum_x; ++x) {
                const double offset_x = static_cast<double>(x) - first.x;
                const double offset_y = static_cast<double>(y) - first.y;
                const double position = length_squared > 0.0
                    ? std::clamp(
                        (offset_x * segment_x + offset_y * segment_y) /
                            length_squared,
                        0.0,
                        1.0
                    )
                    : 0.0;
                const double nearest_x = first.x + position * segment_x;
                const double nearest_y = first.y + position * segment_y;
                const double distance = std::hypot(
                    static_cast<double>(x) - nearest_x,
                    static_cast<double>(y) - nearest_y
                );
                const float amount = static_cast<float>(
                    std::clamp(radius + 0.5 - distance, 0.0, 1.0)
                );
                float &target = coverage[
                    static_cast<std::size_t>(y) * width +
                    static_cast<std::size_t>(x)
                ];
                target = std::max(target, amount);
            }
        }
    }

    std::uint8_t blend_channel(
        std::uint8_t background,
        std::uint8_t foreground,
        double opacity
    ) {
        return static_cast<std::uint8_t>(std::lround(
            static_cast<double>(background) * (1.0 - opacity) +
            static_cast<double>(foreground) * opacity
        ));
    }

    FileData apply_flow_lines(
        FileData data,
        const FlowLineArguments &arguments
    ) {
        if (data.width == 0 || data.height == 0 ||
            arguments.opacity == 0.0) {
            return data;
        }
        const double columns = std::ceil(
            static_cast<double>(data.width) / arguments.spacing
        );
        const double rows = std::ceil(
            static_cast<double>(data.height) / arguments.spacing
        );
        if (columns * rows > 1000000.0) {
            throw std::invalid_argument(
                "flow-lines spacing creates more than 1000000 seeds"
            );
        }

        std::vector<Point> seeds;
        for (double y = arguments.spacing / 2.0;
             y < static_cast<double>(data.height);
             y += arguments.spacing) {
            for (double x = arguments.spacing / 2.0;
                 x < static_cast<double>(data.width);
                 x += arguments.spacing) {
                seeds.push_back({.x = x, .y = y});
            }
        }
        if (seeds.empty()) {
            seeds.push_back({
                .x = (static_cast<double>(data.width) - 1.0) / 2.0,
                .y = (static_cast<double>(data.height) - 1.0) / 2.0,
            });
        }

        ProximityMap occupied(
            data.width,
            data.height,
            arguments.spacing * 0.55
        );
        std::vector<float> coverage(data.pixels.size(), 0.0F);
        for (const Point &seed: seeds) {
            if (!in_bounds(data, seed) || occupied.near(seed)) {
                continue;
            }
            const std::vector<Point> path = trace_streamline(
                data,
                arguments,
                occupied,
                seed
            );
            if (path.size() < 2 ||
                path_length(path) < arguments.spacing * 2.0) {
                continue;
            }
            for (std::size_t index = 1; index < path.size(); ++index) {
                rasterize_segment(
                    coverage,
                    data.width,
                    data.height,
                    path[index - 1],
                    path[index],
                    arguments.width
                );
            }
            occupied.add(path);
        }

        for (std::size_t index = 0; index < data.pixels.size(); ++index) {
            const double opacity =
                static_cast<double>(coverage[index]) * arguments.opacity;
            if (opacity <= 0.0) {
                continue;
            }
            Pixel &pixel = data.pixels[index];
            pixel.red = blend_channel(
                pixel.red,
                arguments.color.red,
                opacity
            );
            pixel.green = blend_channel(
                pixel.green,
                arguments.color.green,
                opacity
            );
            pixel.blue = blend_channel(
                pixel.blue,
                arguments.color.blue,
                opacity
            );
        }
        return data;
    }

    class FlowLinesProcessor final : public ImageProcessor {
    public:
        [[nodiscard]] std::string_view name() const noexcept override {
            return "flow lines";
        }

        [[nodiscard]] std::optional<std::vector<std::string>>
        parse_arguments(std::string_view command) const override {
            auto arguments = parse_command(command);
            if (arguments) {
                validate(*arguments);
            }
            return arguments;
        }

        void validate(
            const std::vector<std::string> &arguments
        ) const override {
            static_cast<void>(parse_flow_line_arguments(arguments));
        }

        [[nodiscard]] FileData apply(
            FileData data,
            const std::vector<std::string> &arguments,
            const MacroMap *macros
        ) const override {
            const MacroMap empty_macros;
            return apply_flow_lines(
                std::move(data),
                parse_flow_line_arguments(
                    arguments,
                    macros != nullptr ? macros : &empty_macros
                )
            );
        }
    };
} // namespace

const ImageProcessor &flow_lines_processor() {
    static const FlowLinesProcessor processor;
    return processor;
}
