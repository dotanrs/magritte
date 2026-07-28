#ifndef MAGRITTE_PARSER_H
#define MAGRITTE_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include "magritte/processors/image_processor.h"

/// Parses and validates one processor command.
///
/// Returns `std::nullopt` for an empty, unknown, or malformed command. When
/// provided, `error_message` receives a user-facing explanation on failure.
[[nodiscard]] std::optional<ProcessorCommand> parse_processor_command(
    std::string_view command,
    std::string *error_message = nullptr
);

#endif //MAGRITTE_PARSER_H
