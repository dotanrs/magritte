#ifndef MAGRITTE_PARSER_H
#define MAGRITTE_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include "magritte/steps/image_step.h"

/// Parses and validates one step command.
///
/// Returns `std::nullopt` for an empty, unknown, or malformed command. When
/// provided, `error_message` receives a user-facing explanation on failure.
[[nodiscard]] std::optional<StepCommand> parse_step_command(
    std::string_view command,
    std::string *error_message = nullptr
);

#endif //MAGRITTE_PARSER_H
