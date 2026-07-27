#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "pixlie/cli.h"
#include "pixlie/drawing_config.h"
#include "pixlie/processor.h"
#include "pixlie/utils/logging.h"

namespace {
    void print_usage(std::ostream &output, std::string_view program) {
        output << "Usage: " << program
                << " <input.jpg> [-o <output.jpg>] [--overwrite]"
                   " [--debug] [-p <processor>]...\n"
                << "       " << program
                << " <drawing.yml> [input.jpg] [-o <output.jpg>]"
                   " [--overwrite] [--debug]\n"
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
                << "  processors:          Named commands; command is the -p value\n"
                << "  Optional input.jpg:  Replaces canvas or source_image entirely\n"
                << "  Optional -o path:    Replaces the configured output path\n";
        output << "\n"
                << "Processors:\n"
                << "  rotate <int>         Rotate clockwise by 90 degrees <int> times\n"
                << "  mirror <x|y>         Reflect across the x-axis or y-axis\n"
                << "  blur <radius>        Box blur with a nonnegative integer radius\n"
                << "  fisheye <x> <y> <amount> [radius]\n"
                << "                       Radial warp using percentage coordinates\n"
                << "  twist <x> <y> <force> [radius]\n"
                << "                       Distance-scaled twist around a percent center\n"
                << "  flow-lines <spacing> <steps> <step> <width> <#RRGGBB>\n"
                << "             [opacity] = (<VX>, <VY>)\n"
                << "                       Draw RK4-traced vector-field streamlines\n"
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

} // namespace

int main(int argc, char *argv[]) {
    const std::string_view program = argc > 0 ? argv[0] : "pixlie";

    if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                      std::string_view(argv[1]) == "--help")) {
        print_usage(std::cout, program);
        return 0;
    }

    try {
        const CommandLineArguments arguments =
            parse_command_line(argc, argv);
        if (arguments.drawing) {
            log(LogLevel::info, "pixlie drawing started");
            process_drawing(
                arguments.input,
                arguments.overwrite,
                arguments.debug,
                arguments.source_override,
                arguments.output
            );
            return 0;
        }
        const Options options{
            .input = arguments.input,
            .output = *arguments.output,
            .processors = arguments.processors,
            .overwrite = arguments.overwrite,
            .debug = arguments.debug,
        };
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
