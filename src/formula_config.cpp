#include "magritte/formula.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {
    struct Line {
        std::size_t number;
        std::size_t indent;
        std::string text;
        std::optional<std::string> block_scalar;
    };

    struct PhysicalLine {
        std::size_t number;
        std::string text;
    };

    struct FormulaStepBuilder {
        std::string name;
        std::string command;
        std::optional<fs::path> formula;
    };

    [[noreturn]] void fail(
        std::string_view source,
        std::size_t line,
        std::string_view message
    ) {
        throw std::invalid_argument(
            std::string(source) + ":" + std::to_string(line) + ": " +
            std::string(message)
        );
    }

    std::string_view trim_view(std::string_view value) {
        while (!value.empty() &&
               std::isspace(static_cast<unsigned char>(value.front()))) {
            value.remove_prefix(1);
        }
        while (!value.empty() &&
               std::isspace(static_cast<unsigned char>(value.back()))) {
            value.remove_suffix(1);
        }
        return value;
    }

    std::pair<std::string_view, std::string_view> split_field(
        std::string_view text,
        std::string_view source,
        std::size_t line
    ) {
        const std::size_t separator = text.find(':');
        if (separator == std::string_view::npos) {
            fail(source, line, "expected a key followed by ':'");
        }
        const std::string_view key = trim_view(text.substr(0, separator));
        if (key.empty()) {
            fail(source, line, "mapping key cannot be empty");
        }
        return {key, trim_view(text.substr(separator + 1))};
    }

    std::string parse_quoted(
        std::string_view value,
        std::string_view source,
        std::size_t line
    ) {
        const char quote = value.front();
        std::string result;
        bool closed = false;

        for (std::size_t index = 1; index < value.size(); ++index) {
            const char character = value[index];
            if (character == quote) {
                if (quote == '\'' && index + 1 < value.size() &&
                    value[index + 1] == '\'') {
                    result.push_back('\'');
                    ++index;
                    continue;
                }
                const std::string_view remainder =
                    trim_view(value.substr(index + 1));
                if (!remainder.empty() && remainder.front() != '#') {
                    fail(source, line, "unexpected text after quoted value");
                }
                closed = true;
                break;
            }
            if (quote == '"' && character == '\\') {
                if (++index >= value.size()) {
                    fail(source, line, "unterminated escape sequence");
                }
                switch (value[index]) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    default:
                        fail(source, line, "unsupported escape sequence");
                }
            } else {
                result.push_back(character);
            }
        }
        if (!closed) {
            fail(source, line, "unterminated quoted value");
        }
        return result;
    }

    std::string parse_scalar(
        std::string_view value,
        std::string_view source,
        std::size_t line
    ) {
        value = trim_view(value);
        if (value.empty()) {
            fail(source, line, "value cannot be empty");
        }
        if (value.front() == '\'' || value.front() == '"') {
            return parse_quoted(value, source, line);
        }

        for (std::size_t index = 0; index < value.size(); ++index) {
            if (value[index] == '#' &&
                (index == 0 ||
                 std::isspace(static_cast<unsigned char>(value[index - 1])))) {
                value = trim_view(value.substr(0, index));
                break;
            }
        }
        if (value.empty()) {
            fail(source, line, "value cannot be empty");
        }
        return std::string(value);
    }

    std::size_t parse_dimension(
        std::string_view value,
        std::string_view source,
        std::size_t line
    ) {
        const std::string scalar = parse_scalar(value, source, line);
        std::size_t result = 0;
        const auto [end, error] = std::from_chars(
            scalar.data(),
            scalar.data() + scalar.size(),
            result
        );
        if (error != std::errc{} || end != scalar.data() + scalar.size() ||
            result == 0) {
            fail(source, line, "dimension must be a positive integer");
        }
        return result;
    }

    std::optional<char> block_scalar_style(std::string_view value) {
        value = trim_view(value);
        if (value.empty() ||
            value.front() == '\'' ||
            value.front() == '"') {
            return std::nullopt;
        }
        for (std::size_t index = 0; index < value.size(); ++index) {
            if (value[index] == '#' &&
                (index == 0 ||
                 std::isspace(static_cast<unsigned char>(value[index - 1])))) {
                value = trim_view(value.substr(0, index));
                break;
            }
        }
        if (value == ">" || value == "|") {
            return value.front();
        }
        return std::nullopt;
    }

    std::string render_block_scalar(
        const std::vector<PhysicalLine> &physical_lines,
        std::size_t begin,
        std::size_t end,
        char style,
        std::string_view source,
        std::size_t header_line
    ) {
        std::optional<std::size_t> content_indent;
        for (std::size_t index = begin; index < end; ++index) {
            const PhysicalLine &line = physical_lines[index];
            const std::size_t first = line.text.find_first_not_of(' ');
            if (first == std::string::npos) {
                continue;
            }
            if (line.text[first] == '\t') {
                fail(
                    source,
                    line.number,
                    "tabs are not supported for indentation"
                );
            }
            if (!content_indent) {
                content_indent = first;
            } else if (first < *content_indent) {
                fail(
                    source,
                    line.number,
                    "invalid block scalar indentation"
                );
            }
        }
        if (!content_indent) {
            fail(source, header_line, "block scalar cannot be empty");
        }

        std::vector<std::string> content;
        std::vector<bool> more_indented;
        content.reserve(end - begin);
        more_indented.reserve(end - begin);
        for (std::size_t index = begin; index < end; ++index) {
            const std::string &text = physical_lines[index].text;
            const std::size_t first = text.find_first_not_of(' ');
            if (first == std::string::npos) {
                content.emplace_back();
                more_indented.push_back(false);
                continue;
            }
            content.push_back(text.substr(*content_indent));
            more_indented.push_back(first > *content_indent);
        }

        std::string result;
        if (style == '|') {
            for (const std::string &line: content) {
                result += line;
                result.push_back('\n');
            }
        } else {
            for (std::size_t index = 0; index < content.size(); ++index) {
                const bool empty = content[index].empty();
                if (index > 0) {
                    const bool previous_empty = content[index - 1].empty();
                    if (empty) {
                        result.push_back('\n');
                    } else if (!previous_empty) {
                        result.push_back(
                            more_indented[index - 1] ||
                            more_indented[index] ? '\n' : ' '
                        );
                    }
                }
                result += content[index];
            }
        }

        while (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        result.push_back('\n');
        return result;
    }

    std::vector<Line> read_lines(
        std::istream &input,
        std::string_view source
    ) {
        std::vector<PhysicalLine> physical_lines;
        std::string text;
        std::size_t number = 0;
        while (std::getline(input, text)) {
            ++number;
            if (!text.empty() && text.back() == '\r') {
                text.pop_back();
            }
            physical_lines.push_back({number, std::move(text)});
        }
        if (!input.eof() && input.fail()) {
            throw std::invalid_argument(
                "could not read formula file: " + std::string(source)
            );
        }

        std::vector<Line> lines;
        for (std::size_t index = 0; index < physical_lines.size();) {
            const PhysicalLine &line = physical_lines[index];
            const std::size_t first = line.text.find_first_not_of(' ');
            if (first == std::string::npos) {
                ++index;
                continue;
            }
            if (line.text[first] == '\t') {
                fail(
                    source,
                    line.number,
                    "tabs are not supported for indentation"
                );
            }
            const std::string_view content =
                trim_view(std::string_view(line.text).substr(first));
            if (content.empty() || content.front() == '#') {
                ++index;
                continue;
            }

            std::optional<char> style;
            const std::size_t separator = content.find(':');
            if (separator != std::string_view::npos) {
                style = block_scalar_style(content.substr(separator + 1));
            }
            if (!style) {
                lines.push_back({
                    line.number,
                    first,
                    std::string(content),
                    std::nullopt,
                });
                ++index;
                continue;
            }

            std::size_t end = index + 1;
            while (end < physical_lines.size()) {
                const PhysicalLine &candidate = physical_lines[end];
                const std::size_t candidate_first =
                    candidate.text.find_first_not_of(' ');
                if (candidate_first == std::string::npos) {
                    ++end;
                    continue;
                }
                if (candidate.text[candidate_first] == '\t') {
                    fail(
                        source,
                        candidate.number,
                        "tabs are not supported for indentation"
                    );
                }
                if (candidate_first <= first) {
                    break;
                }
                ++end;
            }
            lines.push_back({
                line.number,
                first,
                std::string(content),
                render_block_scalar(
                    physical_lines,
                    index + 1,
                    end,
                    *style,
                    source,
                    line.number
                ),
            });
            index = end;
        }
        return lines;
    }

    std::string parse_line_scalar(
        const Line &line,
        std::string_view value,
        std::string_view source
    ) {
        if (line.block_scalar) {
            return *line.block_scalar;
        }
        return parse_scalar(value, source, line.number);
    }

    void set_step_field(
        FormulaStepBuilder &step,
        const Line &line,
        std::string_view key,
        std::string_view value,
        std::string_view source
    ) {
        if (key == "name") {
            if (!step.name.empty()) {
                fail(source, line.number, "duplicate processor name");
            }
            step.name = parse_line_scalar(line, value, source);
        } else if (key == "command") {
            if (!step.command.empty()) {
                fail(source, line.number, "duplicate processor command");
            }
            step.command = parse_line_scalar(line, value, source);
        } else if (key == "formula") {
            if (step.formula) {
                fail(source, line.number, "duplicate formula reference");
            }
            step.formula = parse_line_scalar(line, value, source);
        } else {
            fail(source, line.number, "unknown processor field");
        }
    }

    std::string lowercase_extension(fs::path path) {
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );
        return extension;
    }
} // namespace

FormulaConfig parse_formula_config(
    std::istream &input,
    std::string_view source_name
) {
    const std::vector<Line> lines = read_lines(input, source_name);
    FormulaConfig config{};
    bool has_canvas = false;
    bool has_processors = false;
    bool has_file_name = false;
    bool has_width = false;
    bool has_height = false;

    enum class Section { none, canvas, processors };
    Section section = Section::none;
    std::optional<FormulaStepBuilder> step;

    const auto finish_step = [&]() {
        if (!step) {
            return;
        }
        if (step->formula) {
            if (!step->name.empty() || !step->command.empty()) {
                throw std::invalid_argument(
                    std::string(source_name) +
                    ": a formula reference cannot also define name or command"
                );
            }
            config.steps.emplace_back(FormulaReference{
                .path = std::move(*step->formula),
            });
            step.reset();
            return;
        }
        if (step->name.empty() || step->command.empty()) {
            throw std::invalid_argument(
                std::string(source_name) +
                ": every processor requires name and command"
            );
        }
        config.steps.emplace_back(ProcessorSpec{
            .name = std::move(step->name),
            .command = std::move(step->command),
        });
        step.reset();
    };

    for (const Line &line: lines) {
        if (line.indent == 0) {
            if (section == Section::processors) {
                finish_step();
            }
            const auto [key, value] =
                split_field(line.text, source_name, line.number);
            if (key == "canvas") {
                if (!value.empty()) {
                    fail(
                        source_name,
                        line.number,
                        "canvas must be a mapping"
                    );
                }
                if (has_canvas) {
                    fail(source_name, line.number, "duplicate canvas section");
                }
                has_canvas = true;
                config.canvas.emplace();
                section = Section::canvas;
            } else if (key == "processors") {
                if (!value.empty()) {
                    fail(
                        source_name,
                        line.number,
                        "processors must be a sequence"
                    );
                }
                if (has_processors) {
                    fail(source_name, line.number, "duplicate processors section");
                }
                has_processors = true;
                section = Section::processors;
            } else {
                fail(source_name, line.number, "unknown top-level field");
            }
            continue;
        }

        if (section == Section::canvas) {
            const auto [key, value] =
                split_field(line.text, source_name, line.number);
            if (key == "file_name") {
                if (has_file_name) {
                    fail(source_name, line.number, "duplicate canvas file_name");
                }
                has_file_name = true;
                config.canvas->file_name = parse_scalar(
                    value,
                    source_name,
                    line.number
                );
            } else if (key == "width") {
                if (has_width) {
                    fail(source_name, line.number, "duplicate canvas width");
                }
                has_width = true;
                config.canvas->width = parse_dimension(
                    value,
                    source_name,
                    line.number
                );
            } else if (key == "height") {
                if (has_height) {
                    fail(source_name, line.number, "duplicate canvas height");
                }
                has_height = true;
                config.canvas->height = parse_dimension(
                    value,
                    source_name,
                    line.number
                );
            } else {
                fail(source_name, line.number, "unknown canvas field");
            }
            continue;
        }

        if (section != Section::processors) {
            fail(source_name, line.number, "field appears before a section");
        }

        std::string_view item = line.text;
        if (item.front() == '-') {
            finish_step();
            step.emplace();
            item = trim_view(item.substr(1));
            if (item.empty()) {
                continue;
            }
        } else if (!step) {
            fail(source_name, line.number, "processor must start with '-'");
        }

        const auto [key, value] =
            split_field(item, source_name, line.number);
        set_step_field(
            *step,
            line,
            key,
            value,
            source_name
        );
    }
    if (section == Section::processors) {
        finish_step();
    }

    if (has_canvas && (!has_file_name || !has_width || !has_height)) {
        throw std::invalid_argument(
            std::string(source_name) +
            ": canvas requires file_name, width, and height"
        );
    }
    if (!has_processors) {
        throw std::invalid_argument(
            std::string(source_name) + ": missing processors section"
        );
    }
    if (config.canvas &&
        config.canvas->width > std::numeric_limits<std::size_t>::max() /
                               config.canvas->height) {
        throw std::invalid_argument(
            std::string(source_name) + ": canvas dimensions are too large"
        );
    }
    if (config.canvas) {
        const std::string extension = lowercase_extension(
            config.canvas->file_name
        );
        if (extension != ".jpg" && extension != ".jpeg") {
            throw std::invalid_argument(
                std::string(source_name) +
                ": canvas file_name must end in .jpg or .jpeg"
            );
        }
    }

    return config;
}

FormulaConfig load_formula_config(const fs::path &path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(
            "could not open formula file: " + path.string()
        );
    }
    FormulaConfig config = parse_formula_config(input, path.string());
    if (config.canvas) {
        if (config.canvas->file_name.is_relative()) {
            config.canvas->file_name =
                path.parent_path() / config.canvas->file_name;
        }
        config.canvas->file_name = fs::absolute(
            config.canvas->file_name
        ).lexically_normal();
    }
    for (PipelineStep &step: config.steps) {
        FormulaReference *reference = std::get_if<FormulaReference>(&step);
        if (reference == nullptr) {
            continue;
        }
        if (reference->path.is_relative()) {
            reference->path = path.parent_path() / reference->path;
        }
        reference->path = fs::absolute(reference->path).lexically_normal();
    }
    return config;
}

namespace {
    std::optional<CanvasConfig> expand_formula(
        const fs::path &path,
        std::vector<fs::path> &active_formulas,
        std::vector<ProcessorSpec> &processors
    ) {
        const fs::path normalized =
            fs::absolute(path).lexically_normal();
        if (std::find(
                active_formulas.begin(),
                active_formulas.end(),
                normalized
            ) != active_formulas.end()) {
            throw std::invalid_argument(
                "cyclic formula reference: " + normalized.string()
            );
        }

        active_formulas.push_back(normalized);
        FormulaConfig config = load_formula_config(normalized);
        std::optional<CanvasConfig> canvas = config.canvas;
        for (const PipelineStep &step: config.steps) {
            if (const ProcessorSpec *processor =
                    std::get_if<ProcessorSpec>(&step)) {
                processors.push_back(*processor);
                continue;
            }
            const FormulaReference &reference =
                std::get<FormulaReference>(step);
            std::optional<CanvasConfig> nested_canvas = expand_formula(
                reference.path,
                active_formulas,
                processors
            );
            if (!canvas && nested_canvas) {
                canvas = std::move(nested_canvas);
            }
        }
        active_formulas.pop_back();
        return canvas;
    }
}

ResolvedPipeline resolve_pipeline_steps(
    const std::vector<PipelineStep> &steps,
    bool has_source
) {
    if (!has_source &&
        (steps.empty() ||
         !std::holds_alternative<FormulaReference>(steps.front()))) {
        throw std::invalid_argument(
            "without --source, the first processing argument must be -f"
        );
    }

    ResolvedPipeline resolved;
    std::vector<fs::path> active_formulas;
    for (std::size_t index = 0; index < steps.size(); ++index) {
        if (const ProcessorSpec *processor =
                std::get_if<ProcessorSpec>(&steps[index])) {
            resolved.processors.push_back(*processor);
            continue;
        }
        const FormulaReference &reference =
            std::get<FormulaReference>(steps[index]);
        std::optional<CanvasConfig> canvas = expand_formula(
            reference.path,
            active_formulas,
            resolved.processors
        );
        if (!has_source && index == 0) {
            resolved.canvas = std::move(canvas);
        }
    }

    if (!has_source && !resolved.canvas) {
        throw std::invalid_argument(
            "the first formula must include a canvas when --source is not provided"
        );
    }
    return resolved;
}
