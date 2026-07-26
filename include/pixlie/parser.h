#ifndef PIXLIE_PARSER_H
#define PIXLIE_PARSER_H

#include <optional>
#include <string_view>
#include "pixlie/processors/image_processor.h"

[[nodiscard]] std::optional<ProcessorCommand> parse_processor_command(
    std::string_view command
);

#endif //PIXLIE_PARSER_H
