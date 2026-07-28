#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "magritte/cli.h"
#include "magritte/formula.h"
#include "magritte/utils/logging.h"

namespace {
    void print_usage(std::ostream &output, std::string_view program) {
        output << "Usage: " << program
                << " [--source <source.jpg>]"
                   " [-p <processor> | -f <formula.yml>]..."
                   " [-o <output.jpg>]"
                   " [--debug] [--overwrite]\n"
                << "\n"
                << "Every -p and -f runs in the order supplied.\n"
                << "Without --source, the first processing argument must be\n"
                << "a formula that includes a canvas.\n"
                << "\n"
                << "Options:\n"
                << "  --source <path>      Source JPEG\n"
                << "  -p <command>         Processor command; may be repeated\n"
                << "  -f <path>            Formula YAML; may be repeated\n"
                << "  -o <path>            Destination JPEG\n"
                << "  --debug              Add visual processor hints to the output\n"
                << "  --overwrite          Replace an existing output without prompting\n"
                << "  -h, --help           Show this help message\n";
        output << "\n"
                << "Formula YAML:\n"
                << "  canvas:              Initializes a source-less run\n"
                << "  processors:          Ordered commands or formula references\n"
                << "  - formula: <path>    Include a formula relative to this file\n"
                << "  -o <path>            Replaces the canvas file_name\n";
        output << "\n"
                << "Processors:\n"
                << "  rotate <int>         Rotate clockwise by 90 degrees <int> times\n"
                << "  mirror <x|y>         Reflect across the x-axis or y-axis\n"
                << "  blur <radius>        Box blur with a nonnegative integer radius\n"
                << "  fisheye <x> <y> <amount> [radius]\n"
                << "                       Radial warp using percentage coordinates\n"
                << "  twist <x> <y> <force> [radius]\n"
                << "                       Distance-scaled twist around a percent center\n"
                << "  spin <x> <y> <angle> [radius]\n"
                << "                       Fixed-angle rotation around a percent center\n"
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
    const std::string_view program = argc > 0 ? argv[0] : "magritte";

    if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                      std::string_view(argv[1]) == "--help")) {
        print_usage(std::cout, program);
        return 0;
    }

    try {
        const CommandLineArguments arguments =
            parse_command_line(argc, argv);
        log(LogLevel::info, "magritte started");
        process_pipeline(
            arguments.steps,
            arguments.overwrite,
            arguments.debug,
            arguments.source,
            arguments.output
        );
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
