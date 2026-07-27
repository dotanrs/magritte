#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "pixlie/drawing_config.h"
#include "pixlie/processor.h"
#include "pixlie/utils/logging.h"

namespace fs = std::filesystem;

namespace {
    void print_usage(std::ostream &output, std::string_view program) {
        output << "Usage: " << program
                << " <input.jpg> [-o <output.jpg>] [--overwrite]"
                   " [--debug] [-p <processor>]...\n"
                << "       " << program
                << " <drawing.yml> [--overwrite] [--debug]\n"
                << "\n"
                << "Processes a JPEG, or creates a drawing from a YAML file.\n"
                << "If no output is supplied, <input>_copy.jpg is used.\n"
                << "\n"
                << "Options:\n"
                << "  -o, --output <path>  Destination image path\n"
                << "      --overwrite      Replace an existing output without prompting\n"
                << "  -d, --debug          Add visual processor hints to the output\n"
                << "  -p, --processor <command>\n"
                << "                       Processor command; may be repeated\n"
                << "  -h, --help           Show this help message\n";
        output << "\n"
                << "Drawing YAML:\n"
                << "  canvas:              Creates file_name at width and height\n"
                << "  source_image:        Reads an existing JPEG instead of canvas\n"
                << "  processors:          Named commands; comment is the -p value\n";
        output << "\n"
                << "Processors:\n"
                << "  rotate <int>         Rotate clockwise by 90 degrees <int> times\n"
                << "  mirror <x|y>         Reflect across the x-axis or y-axis\n"
                << "  blur <radius>        Box blur with a nonnegative integer radius\n"
                << "  fisheye <x> <y> <amount> [radius]\n"
                << "                       Radial warp using percentage coordinates\n"
                << "  lighting <preset> [strength]\n"
                << "                       golden-hour, moonlight, studio, or synthwave\n"
                << "  lighting <angle> <#RRGGBB> <threshold|auto>\n"
                << "           [strength [softness [atmosphere]]]\n"
                << "                       Directional gel light with soft occlusion\n"
                << "  <channels> = <formula-or-tuple>\n"
                << "                       Replace any ordered subset of r, g, b\n"
                << "                       e.g. r = G, rg = (G, R), bgr = (R, G, B)\n"
                << "  loop-rgb <n> = (<r>, <g>, <b>)\n"
                << "                       Apply an RGB formula repeatedly\n"
                << "  local-rgb = (<r>, <g>, <b>)\n"
                << "                       RGB formula with neighbor sampling\n"
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
                options.processors.push_back({
                    .name = {},
                    .command = argv[index],
                });
            } else if (argument == "--overwrite") {
                options.overwrite = true;
            } else if (argument == "-d" || argument == "--debug") {
                options.debug = true;
            } else {
                throw std::invalid_argument("unknown argument: " + std::string(argument));
            }
        }

        if (options.output.empty()) {
            options.output = default_output_path(options.input);
        }

        return options;
    }

    bool is_drawing_path(const fs::path &path) {
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );
        return extension == ".yml" || extension == ".yaml";
    }

    std::pair<bool, bool> parse_drawing_arguments(int argc, char *argv[]) {
        bool overwrite = false;
        bool debug = false;
        for (int index = 2; index < argc; ++index) {
            const std::string_view argument = argv[index];
            if (argument == "--overwrite") {
                overwrite = true;
            } else if (argument == "-d" || argument == "--debug") {
                debug = true;
            } else {
                throw std::invalid_argument(
                    "unknown drawing argument: " + std::string(argument)
                );
            }
        }
        return {overwrite, debug};
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
        if (argc >= 2 && is_drawing_path(argv[1])) {
            const auto [overwrite, debug] =
                parse_drawing_arguments(argc, argv);
            log(LogLevel::info, "pixlie drawing started");
            process_drawing(argv[1], overwrite, debug);
            return 0;
        }
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
