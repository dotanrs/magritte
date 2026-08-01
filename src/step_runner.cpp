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


FileData process_image(const MagritteRunOptions &options) {
    auto [commands, results] = parse_steps(options.steps);

    const FileData data = run_steps(
        std::move(options.file_data),
        commands,
        options.debug,
        &options.macros
    );

    log(
        LogLevel::info,
        "Processing complete (" + std::to_string(commands.size()) +
        " steps applied)"
    );
    print_step_results(results);
    return data;
}
