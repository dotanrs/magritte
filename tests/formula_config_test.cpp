#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

#include "common/test_support.h"
#include "magritte/formula.h"
#include "magritte/parser.h"

namespace {
    namespace fs = std::filesystem;

    void expect_invalid(std::string_view yaml, std::string_view message) {
        std::istringstream input{std::string(yaml)};
        try {
            static_cast<void>(parse_formula_config(input, "test.yml"));
            expect(false, std::string(message));
        } catch (const std::invalid_argument &) {
            expect(true, std::string(message));
        }
    }

    void write_formula(
        const fs::path &path,
        std::string_view contents
    ) {
        std::ofstream output(path);
        output << contents;
    }
}

void test_formula_config() {
    std::istringstream input(R"YAML(
# An original generative formula
canvas:
  file_name: "output/field.jpg"
  width: 640
  height: 480
macros:
  macro_wave=60 * sin(D / 8)
processors:
  - name: background
    command: 'rgb = (245, 241, 230)'
  - name: blue field
    command: "rgb = (R, 80 + 60 * sin(D / 8), 180)"
)YAML");

    const FormulaConfig config = parse_formula_config(input, "formula.yml");
    expect(config.canvas.has_value(), "formula has canvas");
    expect(
        config.canvas->file_name == "output/field.jpg",
        "formula output path"
    );
    expect(config.canvas->width == 640, "formula canvas width");
    expect(config.canvas->height == 480, "formula canvas height");
    expect(
        config.macros.at("macro_wave") == "60 * sin(D / 8)",
        "formula parses top-level macros"
    );
    expect(config.steps.size() == 2, "formula processor count");
    expect(
        std::get<ProcessorSpec>(config.steps[1]).name == "blue field",
        "formula processor name"
    );
    expect(
        std::get<ProcessorSpec>(config.steps[1]).command ==
            "rgb = (R, 80 + 60 * sin(D / 8), 180)",
        "formula processor command"
    );

    std::istringstream block_scalars(R"YAML(
processors:
  - name: folded command
    command: > # Fold physical lines into spaces.
      local-rgb =
      (
      R,
      G,
      B
      )
  - name: literal command
    command: |
      rgb =
      (
      R,
      G,
      B
      )
)YAML");
    const FormulaConfig block_scalar_config =
        parse_formula_config(block_scalars, "blocks.yml");
    expect(
        block_scalar_config.steps.size() == 2,
        "formula accepts folded and literal block scalars"
    );
    expect(
        std::get<ProcessorSpec>(block_scalar_config.steps[0]).command ==
            "local-rgb = ( R, G, B )\n",
        "folded block scalar replaces ordinary line breaks with spaces"
    );
    expect(
        std::get<ProcessorSpec>(block_scalar_config.steps[1]).command ==
            "rgb =\n(\nR,\nG,\nB\n)\n",
        "literal block scalar preserves line breaks"
    );
    expect(
        parse_processor_command(
            std::get<ProcessorSpec>(
                block_scalar_config.steps[0]
            ).command
        ).has_value(),
        "folded block scalar produces a valid processor command"
    );
    expect(
        parse_processor_command(
            std::get<ProcessorSpec>(
                block_scalar_config.steps[1]
            ).command
        ).has_value(),
        "literal block scalar produces a valid processor command"
    );
    expect_invalid(
        "processors:\n"
        "  - name: empty block\n"
        "    command: >\n",
        "formula rejects an empty block scalar"
    );
    expect_invalid(
        "processors:\n"
        "  - name: inconsistent indentation\n"
        "    command: >\n"
        "      rgb =\n"
        "     (R, G, B)\n",
        "formula rejects inconsistent block scalar indentation"
    );

    expect_invalid(
        "canvas:\n  file_name: x.jpg\n  width: 0\n  height: 5\n"
        "processors:\n",
        "formula rejects zero dimensions"
    );
    expect_invalid(
        "canvas:\n  file_name: x.png\n  width: 5\n  height: 5\n"
        "processors:\n",
        "formula rejects non-JPEG output"
    );
    expect_invalid(
        "canvas:\n  file_name: x.jpg\n  width: 5\n  height: 5\n"
        "processors:\n  - name: incomplete\n",
        "formula requires each processor command"
    );

    std::istringstream processors_only(R"YAML(
processors:
  - name: soften
    command: "blur 2"
)YAML");
    const FormulaConfig processors_config =
        parse_formula_config(processors_only, "processors.yml");
    expect(
        !processors_config.canvas &&
        processors_config.steps.size() == 1,
        "formula without canvas is valid when CLI source can supply the image"
    );

    expect_invalid(
        "source_image: input.jpg\n"
        "processors:\n",
        "formula rejects the obsolete source_image field"
    );
    expect_invalid(
        "macros:\n"
        "  gain=2\n"
        "processors:\n",
        "formula requires the explicit macro_ prefix"
    );
    expect_invalid(
        "macros:\n"
        "  macro_gain=2\n"
        "  macro_gain=3\n"
        "processors:\n",
        "formula rejects conflicting macros in one file"
    );

    std::istringstream composed(R"YAML(
processors:
  - name: before
    command: "rotate 1"
  - formula: "soften.yml"
  - name: after
    command: "contrast 1.2"
)YAML");
    const FormulaConfig composed_config =
        parse_formula_config(composed, "composed.yml");
    expect(
        composed_config.steps.size() == 3 &&
        std::get<ProcessorSpec>(composed_config.steps[0]).command ==
            "rotate 1" &&
        std::get<FormulaReference>(composed_config.steps[1]).path ==
            "soften.yml" &&
        std::get<ProcessorSpec>(composed_config.steps[2]).command ==
            "contrast 1.2",
        "sub-formulas should retain their position between processors"
    );

    expect_invalid(
        "processors:\n"
        "  - name: invalid\n"
        "    formula: nested.yml\n",
        "formula references cannot also define a processor name"
    );

    const fs::path temp_directory =
        fs::temp_directory_path() /
        (
            "magritte-formula-test-" +
            std::to_string(
                std::chrono::steady_clock::now()
                    .time_since_epoch()
                    .count()
            )
        );
    fs::create_directories(temp_directory);
    write_formula(
        temp_directory / "child.yml",
        "macros:\n"
        "  macro_child=2\n"
        "processors:\n"
        "  - name: second\n"
        "    command: \"blur 1\"\n"
    );
    write_formula(
        temp_directory / "root.yml",
        "canvas:\n"
        "  file_name: output.jpg\n"
        "  width: 4\n"
        "  height: 3\n"
        "macros:\n"
        "  macro_root=macro_cli + 1\n"
        "processors:\n"
        "  - name: first\n"
        "    command: \"rotate 1\"\n"
        "  - formula: child.yml\n"
        "  - name: third\n"
        "    command: \"contrast 1.1\"\n"
    );

    const ResolvedPipeline resolved = resolve_pipeline_steps(
        {
            FormulaReference{.path = temp_directory / "root.yml"},
            ProcessorSpec{.name = {}, .command = "mirror y"},
        },
        false,
        MacroMap{{"macro_cli", "3"}}
    );
    expect(
        resolved.canvas.has_value() &&
        resolved.canvas->width == 4 &&
        resolved.canvas->height == 3,
        "first formula canvas should initialize a source-less pipeline"
    );
    expect(
        resolved.processors.size() == 4 &&
        resolved.processors[0].command == "rotate 1" &&
        resolved.processors[1].command == "blur 1" &&
        resolved.processors[2].command == "contrast 1.1" &&
        resolved.processors[3].command == "mirror y",
        "nested formulas and CLI processors should resolve in exact order"
    );
    expect(
        resolved.macros.size() == 3 &&
        resolved.macros.at("macro_cli") == "3" &&
        resolved.macros.at("macro_root") == "macro_cli + 1" &&
        resolved.macros.at("macro_child") == "2",
        "CLI and nested formula macros should be collected before processing"
    );

    write_formula(
        temp_directory / "wrapper.yml",
        "processors:\n"
        "  - formula: root.yml\n"
    );
    const ResolvedPipeline wrapped = resolve_pipeline_steps(
        {
            FormulaReference{.path = temp_directory / "wrapper.yml"},
        },
        false
    );
    expect(
        wrapped.canvas.has_value() &&
        wrapped.canvas->width == 4 &&
        wrapped.processors.size() == 3,
        "a first formula may inherit its canvas from a sub-formula"
    );

    write_formula(
        temp_directory / "macro-conflict.yml",
        "macros:\n"
        "  macro_child=9\n"
        "processors:\n"
        "  - formula: child.yml\n"
    );
    try {
        static_cast<void>(resolve_pipeline_steps(
            {
                FormulaReference{
                    .path = temp_directory / "macro-conflict.yml",
                },
            },
            true
        ));
        expect(false, "nested formulas should reject conflicting macros");
    } catch (const std::invalid_argument &) {
        expect(true, "nested formulas should reject conflicting macros");
    }

    write_formula(
        temp_directory / "cycle-a.yml",
        "processors:\n"
        "  - formula: cycle-b.yml\n"
    );
    write_formula(
        temp_directory / "cycle-b.yml",
        "processors:\n"
        "  - formula: cycle-a.yml\n"
    );
    try {
        static_cast<void>(resolve_pipeline_steps(
            {
                FormulaReference{
                    .path = temp_directory / "cycle-a.yml",
                },
            },
            true
        ));
        expect(false, "cyclic sub-formulas should be rejected");
    } catch (const std::invalid_argument &) {
        expect(true, "cyclic sub-formulas should be rejected");
    }
    fs::remove_all(temp_directory);
}
