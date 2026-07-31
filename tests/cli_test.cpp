#include <initializer_list>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "common/test_support.h"
#include "magritte/cli.h"

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
        "magritte",
        "--source",
        "input.jpg",
        "-o",
        "output.jpg",
        "-p",
        "blur 2",
        "--macro",
        "macro_gain=1.25",
        "--debug",
    });
    expect(
        source.source == "input.jpg" &&
        source.output == "output.jpg" &&
        source.steps.size() == 1 &&
        std::get<ProcessorSpec>(source.steps[0]).command == "blur 2" &&
        source.macros.at("macro_gain") == "1.25" &&
        source.debug,
        "source-only CLI should parse processing options"
    );

    const CommandLineArguments pattern = parse({
        "magritte",
        "-P",
        "patterns/canvas.yml",
    });
    expect(
        !pattern.source &&
        pattern.steps.size() == 1 &&
        std::get<PatternReference>(pattern.steps[0]).path ==
            "patterns/canvas.yml",
        "pattern-only CLI should parse a pattern file"
    );

    const CommandLineArguments combined = parse({
        "magritte",
        "--source",
        "source.jpg",
        "-p",
        "rotate 1",
        "--pattern",
        "patterns/soften.yml",
        "-p",
        "blur 1",
        "-P",
        "patterns/color.yml",
        "--overwrite",
    });
    expect(
        combined.source == "source.jpg" &&
        combined.steps.size() == 4 &&
        std::get<ProcessorSpec>(combined.steps[0]).command == "rotate 1" &&
        std::get<PatternReference>(combined.steps[1]).path ==
            "patterns/soften.yml" &&
        std::get<ProcessorSpec>(combined.steps[2]).command == "blur 1" &&
        std::get<PatternReference>(combined.steps[3]).path ==
            "patterns/color.yml" &&
        combined.overwrite,
        "processors and patterns should retain CLI order"
    );

    try {
        static_cast<void>(parse({"magritte", "-p", "blur 1"}));
        expect(false, "CLI should require a pattern first without a source");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should require a pattern first without a source");
    }

    try {
        static_cast<void>(parse({"magritte"}));
        expect(false, "CLI should require an input image or pattern");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should require an input image or pattern");
    }

    try {
        static_cast<void>(parse({"magritte", "input.jpg"}));
        expect(false, "CLI should reject positional input");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject positional input");
    }

    try {
        static_cast<void>(parse({"magritte", "--file", "old.yml"}));
        expect(false, "CLI should reject the obsolete --file option");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject the obsolete --file option");
    }

    try {
        static_cast<void>(parse({"magritte", "-f", "legacy.yml"}));
        expect(false, "CLI should reject the obsolete formula flag");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject the obsolete formula flag");
    }

    try {
        static_cast<void>(parse({
            "magritte",
            "--source",
            "input.jpg",
            "--macro",
            "gain=2",
        }));
        expect(false, "CLI should require the explicit macro_ prefix");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should require the explicit macro_ prefix");
    }

    try {
        static_cast<void>(parse({
            "magritte",
            "--source",
            "input.jpg",
            "--macro",
            "macro_gain=2",
            "--macro",
            "macro_gain=3",
        }));
        expect(false, "CLI should reject conflicting macro definitions");
    } catch (const std::invalid_argument &) {
        expect(true, "CLI should reject conflicting macro definitions");
    }

    const CommandLineArguments repeated_macro = parse({
        "magritte",
        "--source",
        "input.jpg",
        "--macro",
        "macro_gain=2",
        "--macro",
        "macro_gain=2",
    });
    expect(
        repeated_macro.macros.size() == 1,
        "CLI should accept repeated identical macro definitions"
    );
}
