//
// Created by Dotan Reis on 26/07/2026.
//

#include "magritte/processor.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../include/magritte/utils/file.h"
#include "../include/magritte/utils/input_validation.h"
#include "magritte/parser.h"
#include "magritte/processors/image_processor.h"
#include "magritte/utils/logging.h"

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

    struct ProcessorErrorError {
        std::string command;
        std::string message;
    };

    struct ProcessorParseResults {
        std::vector<std::string> successful;
        std::vector<ProcessorErrorError> errors;
    };

    std::string processor_description(const ProcessorSpec &processor) {
        if (processor.name.empty()) {
            return processor.command;
        }
        return processor.name + " (" + processor.command + ")";
    }

    std::pair<std::vector<ProcessorCommand>, ProcessorParseResults> parse_processors(
        const std::vector<ProcessorSpec> &processors
    ) {
        ProcessorParseResults results;
        std::vector<ProcessorCommand> commands;
        commands.reserve(processors.size());
        for (const ProcessorSpec &processor: processors) {
            std::string error_message;
            if (auto command = parse_processor_command(
                processor.command,
                &error_message
            )) {
                results.successful.push_back(
                    processor_description(processor)
                );
                commands.push_back(std::move(*command));
            } else {
                results.errors.push_back({
                    .command = processor_description(processor),
                    .message = std::move(error_message),
                });
            }
        }
        return {std::move(commands), std::move(results)};
    }

    void print_processor_results(const ProcessorParseResults &results) {
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
            for (const ProcessorErrorError &error: results.errors) {
                std::clog << "  ! " << error.command << ": " << error.message << '\n';
            }
            std::clog << reset;
        }
    }

    FileData run_processors(
        FileData data,
        const std::vector<ProcessorCommand> &commands,
        bool debug,
        const MacroMap *macros
    ) {
        for (const ProcessorCommand &command: commands) {
            log(
                LogLevel::info,
                "Applying processor: " + std::string(command.processor.get().name())
            );
            data = command.processor.get().apply(
                std::move(data),
                command.arguments,
                macros
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
} // namespace


void process_image(const MagritteRunOptions &options) {
    auto [input, output] = validate_input(options);
    if (!should_write_output(output, options.overwrite)) {
        log(LogLevel::info, "Output file was not overwritten");
        return;
    }

    auto [commands, results] = parse_processors(options.processors);

    const FileData data = run_processors(
        read_file(input),
        commands,
        options.debug,
        &options.macros
    );

    save_file(output, data);
    log(
        LogLevel::info,
        "Processing complete (" + std::to_string(commands.size()) +
        " processors applied)"
    );
    print_processor_results(results);
    std::clog << "\nFile saved to " + output.filename().string() << '\n';
}

void process_created_image(
    const fs::path &output,
    FileData data,
    const std::vector<ProcessorSpec> &processors,
    bool overwrite,
    bool debug,
    const MacroMap &macros
) {
    const fs::path normalized_output =
            fs::absolute(output).lexically_normal();
    if (!should_write_output(normalized_output, overwrite)) {
        log(LogLevel::info, "Output file was not overwritten");
        return;
    }

    auto [commands, results] = parse_processors(processors);
    data = run_processors(std::move(data), commands, debug, &macros);
    save_file(normalized_output, data);
    log(
        LogLevel::info,
        "Generated image complete (" + std::to_string(commands.size()) +
        " processors applied)"
    );
    print_processor_results(results);
    log(LogLevel::info,
        "File saved to " + normalized_output.filename().string());
}
