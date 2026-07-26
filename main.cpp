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

void print_usage(std::ostream& output, std::string_view program) {
    output << "Usage: " << program << " <input.jpg> [-o <output.jpg>]\n"
           << "\n"
           << "Creates an exact copy of a JPEG image.\n"
           << "If no output is supplied, <input>_copy.jpg is used.\n"
           << "\n"
           << "Options:\n"
           << "  -o, --output <path>  Destination image path\n"
           << "  -h, --help           Show this help message\n";
}

fs::path default_output_path(const fs::path& input) {
    return input.parent_path() / (input.stem().string() + "_copy" + input.extension().string());
}

Options parse_arguments(int argc, char* argv[]) {
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
        } else {
            throw std::invalid_argument("unknown argument: " + std::string(argument));
        }
    }

    if (options.output.empty()) {
        options.output = default_output_path(options.input);
    }

    return options;
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string_view program = argc > 0 ? argv[0] : "pixlie";

    if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                      std::string_view(argv[1]) == "--help")) {
        print_usage(std::cout, program);
        return 0;
    }

    try {
        const Options options = parse_arguments(argc, argv);
        log(LogLevel::info, "pixlie started1");
        copy_image(options);
        return 0;
    } catch (const std::invalid_argument& error) {
        log(LogLevel::error, error.what());
        print_usage(std::cerr, program);
        return 2;
    } catch (const std::exception& error) {
        log(LogLevel::error, error.what());
        return 1;
    }
}
