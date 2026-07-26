#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include "utils/logging.h"

namespace fs = std::filesystem;

namespace {


struct Options {
    fs::path input;
    fs::path output;
};

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

bool has_jpeg_markers(const fs::path& path) {
    std::ifstream image(path, std::ios::binary);
    if (!image) {
        throw std::runtime_error("could not open input image");
    }

    std::array<std::uint8_t, 2> start{};
    image.read(reinterpret_cast<char*>(start.data()), static_cast<std::streamsize>(start.size()));
    if (image.gcount() != static_cast<std::streamsize>(start.size())) {
        return false;
    }

    image.seekg(-2, std::ios::end);
    if (!image) {
        return false;
    }

    std::array<std::uint8_t, 2> end{};
    image.read(reinterpret_cast<char*>(end.data()), static_cast<std::streamsize>(end.size()));

    return start[0] == 0xFF && start[1] == 0xD8 &&
           end[0] == 0xFF && end[1] == 0xD9;
}

void validate_input(const fs::path& input) {
    std::error_code error;
    if (!fs::exists(input, error) || error) {
        throw std::runtime_error("input image does not exist: " + input.string());
    }
    if (!fs::is_regular_file(input, error) || error) {
        throw std::runtime_error("input path is not a regular file: " + input.string());
    }
    if (!has_jpeg_markers(input)) {
        throw std::runtime_error("input is not a valid JPEG file: " + input.string());
    }
}

void copy_image(const Options& options) {
    const fs::path input = fs::absolute(options.input).lexically_normal();
    const fs::path output = fs::absolute(options.output).lexically_normal();

    if (input == output) {
        throw std::runtime_error("input and output paths must be different");
    }

    log(LogLevel::info, "Reading JPEG: " + input.string());
    validate_input(input);

    if (!output.parent_path().empty()) {
        std::error_code error;
        fs::create_directories(output.parent_path(), error);
        if (error) {
            throw std::runtime_error("could not create output directory: " + error.message());
        }
    }

    log(LogLevel::info, "Writing copy: " + output.string());
    std::error_code error;
    fs::copy_file(input, output, fs::copy_options::overwrite_existing, error);
    if (error) {
        throw std::runtime_error("could not copy image: " + error.message());
    }

    const auto bytes = fs::file_size(output, error);
    if (error) {
        throw std::runtime_error("could not read output size: " + error.message());
    }
    log(LogLevel::info, "Copy complete (" + std::to_string(bytes) + " bytes)");
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
