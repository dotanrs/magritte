#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include "pixlie/utils/logging.h"
#include "pixlie/processor.h"

namespace fs = std::filesystem;

namespace {
    void print_usage(std::ostream &output, std::string_view program) {
        output << "Usage: " << program
                << " <input.jpg> [-o <output.jpg>] [--overwrite]"
                   " [-p <processor>]...\n"
                << "\n"
                << "Processes a JPEG image using commands in the order provided.\n"
                << "If no output is supplied, <input>_copy.jpg is used.\n"
                << "\n"
                << "Options:\n"
                << "  -o, --output <path>  Destination image path\n"
                << "      --overwrite      Replace an existing output without prompting\n"
                << "  -p, --processor <command>\n"
                << "                       Processor command; may be repeated\n"
                << "  -h, --help           Show this help message\n";
        output << "\n"
                << "Processors:\n"
                << "  rotate <int>         Rotate clockwise by 90 degrees <int> times\n"
                << "  mirror <x|y>         Reflect across the x-axis or y-axis\n"
                << "  blur <radius>        Box blur with a nonnegative integer radius\n"
                << "  fisheye <x> <y> <amount>\n"
                << "                       Magnify (>0) or shrink (-1..0) around x,y\n"
                << "  r = <formula>        Replace red using R, G, and B variables\n"
                << "  g = <formula>        Replace green using R, G, and B variables\n"
                << "  b = <formula>        Replace blue using R, G, and B variables\n"
                << "  s = <formula>        Replace HSL saturation using the S variable\n";
    }

    fs::path default_output_path(const fs::path &input) {
        return input.parent_path() / (input.stem().string() + "_copy" + input.extension().string());
    }

    Options parse_arguments(int argc, char *argv[]) {
        if (argc < 2) {
            throw std::invalid_argument("missing input image");
        }

        Options options;
        options.input = argv[1];

        for (int index = 2; index < argc; ++index) {
            const std::string_view argument = argv[index];
            if (argument == "-o" || argument == "--output") {
                if (++index >= argc) {
                    throw std::invalid_argument("missing path after " + std::string(argument));
                }
                options.output = argv[index];
            } else if (argument == "-p" || argument == "--processor") {
                if (++index >= argc) {
                    throw std::invalid_argument("missing command after " + std::string(argument));
                }
                options.processor_commands.emplace_back(argv[index]);
            } else if (argument == "--overwrite") {
                options.overwrite = true;
            } else {
                throw std::invalid_argument("unknown argument: " + std::string(argument));
            }
        }

        if (options.output.empty()) {
            options.output = default_output_path(options.input);
        }

        return options;
    }
} // namespace

int main(int argc, char *argv[]) {
    const std::string_view program = argc > 0 ? argv[0] : "pixlie";

    if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                      std::string_view(argv[1]) == "--help")) {
        print_usage(std::cout, program);
        return 0;
    }

    try {
        const Options options = parse_arguments(argc, argv);
        log(LogLevel::info, "pixlie started");
        process_image(options);
        return 0;
    } catch (const std::invalid_argument &error) {
        log(LogLevel::error, error.what());
        print_usage(std::cerr, program);
        return 2;
    } catch (const std::exception &error) {
        log(LogLevel::error, error.what());
        return 1;
    }
}
