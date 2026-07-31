//
// Created by Dotan Reis on 26/07/2026.
//

#include "magritte/step.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "magritte/io/file.h"
#include "magritte/io/input_validation.h"
#include "magritte/parser.h"
#include "magritte/steps/image_step.h"
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

    struct StepError {
        std::string command;
        std::string message;
    };

    struct StepParseResults {
        std::vector<std::string> successful;
        std::vector<StepError> errors;
    };

    std::string step_description(const StepSpec &step) {
        if (step.name.empty()) {
            return step.command;
        }
        return step.name + " (" + step.command + ")";
    }

    std::pair<std::vector<StepCommand>, StepParseResults> parse_steps(
        const std::vector<StepSpec> &steps
    ) {
        StepParseResults results;
        std::vector<StepCommand> commands;
        commands.reserve(steps.size());
        for (const StepSpec &step: steps) {
            std::string error_message;
            if (auto command = parse_step_command(
                step.command,
                &error_message
            )) {
                results.successful.push_back(
                    step_description(step)
                );
                commands.push_back(std::move(*command));
            } else {
                results.errors.push_back({
                    .command = step_description(step),
                    .message = std::move(error_message),
                });
            }
        }
        return {std::move(commands), std::move(results)};
    }

    void print_step_results(const StepParseResults &results) {
        if (results.successful.empty() && results.errors.empty()) {
            return;
        }

        constexpr std::string_view green = "\033[32m";
        constexpr std::string_view yellow = "\033[33m";
        constexpr std::string_view reset = "\033[0m";

        std::clog << '\n' << green << "Successful steps:\n";
        if (results.successful.empty()) {
            std::clog << "  (none)\n";
        } else {
            for (const std::string &command: results.successful) {
                std::clog << "  \u2713 " << command << '\n';
            }
        }
        std::clog << reset;

        if (!results.errors.empty()) {
            std::clog << yellow << "Step errors:\n";
            for (const StepError &error: results.errors) {
                std::clog << "  ! " << error.command << ": " << error.message << '\n';
            }
            std::clog << reset;
        }
    }

    FileData run_steps(
        FileData data,
        const std::vector<StepCommand> &commands,
        bool debug,
        const MacroMap *macros
    ) {
        for (const StepCommand &command: commands) {
            log(
                LogLevel::info,
                "Applying step: " + std::string(command.step.get().name())
            );
            data = command.step.get().apply(
                std::move(data),
                command.arguments,
                macros
            );
            if (debug) {
                data = command.step.get().add_debug_hints(
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

    auto [commands, results] = parse_steps(options.steps);

    const FileData data = run_steps(
        read_file(input),
        commands,
        options.debug,
        &options.macros
    );

    save_file(output, data);
    log(
        LogLevel::info,
        "Processing complete (" + std::to_string(commands.size()) +
        " steps applied)"
    );
    print_step_results(results);
    std::clog << "\nFile saved to " + output.filename().string() << '\n';
}

void process_created_image(
    const fs::path &output,
    FileData data,
    const std::vector<StepSpec> &steps,
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

    auto [commands, results] = parse_steps(steps);
    data = run_steps(std::move(data), commands, debug, &macros);
    save_file(normalized_output, data);
    log(
        LogLevel::info,
        "Generated image complete (" + std::to_string(commands.size()) +
        " steps applied)"
    );
    print_step_results(results);
    log(LogLevel::info,
        "File saved to " + normalized_output.filename().string());
}
