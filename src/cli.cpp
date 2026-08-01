#include "magritte/cli.h"

#include <stdexcept>
#include <string>
#include <string_view>

#include "magritte/input_parsing/macro_parser.h"

namespace fs = std::filesystem;

namespace {
    fs::path option_path(
        int &index,
        int argc,
        char *argv[],
        std::string_view option
    ) {
        if (++index >= argc) {
            throw std::invalid_argument(
                "missing path after " + std::string(option)
            );
        }
        return fs::path(argv[index]);
    }
}

CommandLineArguments parse_command_line(int argc, char *argv[]) {
    CommandLineArguments arguments;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--source") {
            if (arguments.source) {
                throw std::invalid_argument(
                    "source image was supplied more than once"
                );
            }
            arguments.source =
                option_path(index, argc, argv, argument);
        } else if (argument == "-o") {
            if (arguments.output) {
                throw std::invalid_argument(
                    "output path was supplied more than once"
                );
            }
            arguments.output =
                option_path(index, argc, argv, argument);
        } else if (argument == "-s" || argument == "--step") {
            if (++index >= argc) {
                throw std::invalid_argument(
                    "missing command after " + std::string(argument)
                );
            }
            arguments.steps.emplace_back(StepSpec{
                .name = {},
                .command = argv[index],
            });
        } else if (argument == "-p" || argument == "--pattern") {
            arguments.steps.emplace_back(PatternReference{
                .path = option_path(index, argc, argv, argument),
            });
        } else if (argument == "--macro") {
            if (++index >= argc) {
                throw std::invalid_argument(
                    "missing definition after --macro"
                );
            }
            auto [name, value] = parse_macro_definition(argv[index]);
            add_macro(
                arguments.macros,
                std::move(name),
                std::move(value)
            );
        } else if (argument == "--overwrite") {
            arguments.overwrite = true;
        } else if (argument == "--debug") {
            arguments.debug = true;
        } else {
            throw std::invalid_argument(
                "unknown argument: " + std::string(argument)
            );
        }
    }

    if (!arguments.source && arguments.steps.empty()) {
        throw std::invalid_argument(
            "an input image or pattern is required"
        );
    }
    if (!arguments.source &&
        !std::holds_alternative<PatternReference>(arguments.steps.front())) {
        throw std::invalid_argument(
            "without --source, the first processing argument must be "
            "-p or --pattern"
        );
    }

    return arguments;
}
