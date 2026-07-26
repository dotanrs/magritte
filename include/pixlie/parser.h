#ifndef PIXLIE_PARSER_H
#define PIXLIE_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include "pixlie/processors/image_processor.h"

[[nodiscard]] std::optional<ProcessorCommand> parse_processor_command(
    std::string_view command,
    std::string* error_message = nullptr
);

#endif //PIXLIE_PARSER_H
