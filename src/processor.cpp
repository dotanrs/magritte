//
// Created by Dotan Reis on 26/07/2026.
//

#include "pixlie/processor.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../include/pixlie/utils/file.h"
#include "pixlie/input_validation.h"
#include "pixlie/parser.h"
#include "pixlie/processors/image_processor.h"
#include "pixlie/utils/logging.h"

namespace {
    bool confirm_overwrite(const fs::path &output) {
        std::cerr << "Output file already exists: " << output.string() << '\n'
                  << "Continue and overwrite it? [y/N] " << std::flush;

        std::string response;
        if (!std::getline(std::cin, response)) {
            std::cerr << '\n';
            return false;
        }

        response.erase(
            response.begin(),
            std::find_if(
                response.begin(),
                response.end(),
                [](unsigned char character) {
                    return !std::isspace(character);
                }
            )
        );
        response.erase(
            std::find_if(
                response.rbegin(),
                response.rend(),
                [](unsigned char character) {
                    return !std::isspace(character);
                }
            ).base(),
            response.end()
        );
        std::transform(
            response.begin(),
            response.end(),
            response.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );
        return response == "y" || response == "yes";
    }

    bool should_write_output(const fs::path &output, bool overwrite) {
        std::error_code error;
        const bool output_exists = fs::exists(output, error);
        if (error) {
            throw std::runtime_error(
                "could not inspect output path: " + error.message()
            );
        }
        return !output_exists || overwrite || confirm_overwrite(output);
    }

    struct ProcessorError {
        std::string command;
        std::string message;
    };

    struct ProcessorResults {
        std::vector<std::string> successful;
        std::vector<ProcessorError> errors;
    };

    void print_processor_results(const ProcessorResults &results) {
        if (results.successful.empty() && results.errors.empty()) {
            return;
        }

        constexpr std::string_view green = "\033[32m";
        constexpr std::string_view yellow = "\033[33m";
        constexpr std::string_view reset = "\033[0m";

        std::clog << '\n' << green << "Successful processors:\n";
        if (results.successful.empty()) {
            std::clog << "  (none)\n";
        } else {
            for (const std::string &command: results.successful) {
                std::clog << "  \u2713 " << command << '\n';
            }
        }
        std::clog << reset;

        if (!results.errors.empty()) {
            std::clog << yellow << "Processor errors:\n";
            for (const ProcessorError &error: results.errors) {
                std::clog << "  ! " << error.command << ": " << error.message << '\n';
            }
            std::clog << reset;
        }
    }
} // namespace

FileData process_file(
    FileData data,
    const std::vector<ProcessorCommand> &commands,
    bool debug
) {
    for (const ProcessorCommand &command: commands) {
        log(
            LogLevel::info,
            "Applying processor: " + std::string(command.processor.get().name())
        );
        data = command.processor.get().apply(
            std::move(data),
            command.arguments
        );
        if (debug) {
            data = command.processor.get().add_debug_hints(
                std::move(data),
                command.arguments
            );
        }
        validate_file_data(data);
    }
    return data;
}

void process_image(const Options &options) {
    auto [input, output] = validate_input(options);
    if (!should_write_output(output, options.overwrite)) {
        log(LogLevel::info, "Output file was not overwritten");
        return;
    }

    ProcessorResults results;
    std::vector<ProcessorCommand> commands;
    commands.reserve(options.processor_commands.size());
    for (const std::string &command_text: options.processor_commands) {
        std::string error_message;
        if (auto command = parse_processor_command(command_text, &error_message)) {
            commands.push_back(std::move(*command));
        } else {
            results.errors.push_back({
                .command = command_text,
                .message = std::move(error_message),
            });
        }
    }

    const FileData data = process_file(read_file(input), commands, options.debug);
    for (const ProcessorCommand &command: commands) {
        results.successful.push_back(command.source);
    }

    save_file(output, data);
    log(
        LogLevel::info,
        "Processing complete (" + std::to_string(commands.size()) +
        " processors applied)"
    );
    print_processor_results(results);
}
