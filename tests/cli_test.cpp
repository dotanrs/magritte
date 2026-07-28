#include <initializer_list>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "common/test_support.h"
#include "pixlie/cli.h"

namespace {
    CommandLineArguments parse(
        std::initializer_list<std::string> values
    ) {
        std::vector<std::string> storage(values);
        std::vector<char *> arguments;
        arguments.reserve(storage.size());
        for (std::string &value: storage) {
            arguments.push_back(value.data());
        }
        return parse_command_line(
            static_cast<int>(arguments.size()),
            arguments.data()
        );
    }
}

void test_cli_arguments() {
    const CommandLineArguments source = parse({
        "pixlie",
        "--source",
        "input.jpg",
        "-o",
        "output.jpg",
        "-p",
        "blur 2",
        "--debug",
    });
    expect(
        source.source == "input.jpg" &&
        source.output == "output.jpg" &&
        source.steps.size() == 1 &&
        std::get<ProcessorSpec>(source.steps[0]).command == "blur 2" &&
        source.debug,
        "source-only CLI should parse processing options"
    );

    const CommandLineArguments formula = parse({
        "pixlie",
        "-f",
        "formulas/canvas.yml",
    });
    expect(
        !formula.source &&
        formula.steps.size() == 1 &&
        std::get<FormulaReference>(formula.steps[0]).path ==
            "formulas/canvas.yml",
        "formula-only CLI should parse a formula file"
    );

    const CommandLineArguments combined = parse({
        "pixlie",
        "--source",
        "source.jpg",
        "-p",
        "rotate 1",
        "-f",
        "formulas/soften.yml",
        "-p",
        "blur 1",
        "-f",
        "formulas/color.yml",
        "--overwrite",
    });
    expect(
        combined.source == "source.jpg" &&
        combined.steps.size() == 4 &&
        std::get<ProcessorSpec>(combined.steps[0]).command == "rotate 1" &&
        std::get<FormulaReference>(combined.steps[1]).path ==
            "formulas/soften.yml" &&
        std::get<ProcessorSpec>(combined.steps[2]).command == "blur 1" &&
        std::get<FormulaReference>(combined.steps[3]).path ==
            "formulas/color.yml" &&
        combined.overwrite,
        "processors and formulas should retain CLI order"
    );

    try {
        static_cast<void>(parse({"pixlie", "-p", "blur 1"}));
        expect(false, "CLI should require a formula first without a source");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should require a formula first without a source");
    }

    try {
        static_cast<void>(parse({"pixlie"}));
        expect(false, "CLI should require an input image or formula");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should require an input image or formula");
    }

    try {
        static_cast<void>(parse({"pixlie", "input.jpg"}));
        expect(false, "CLI should reject positional input");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject positional input");
    }

    try {
        static_cast<void>(parse({"pixlie", "--file", "old.yml"}));
        expect(false, "CLI should reject the obsolete --file option");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject the obsolete --file option");
    }
}
