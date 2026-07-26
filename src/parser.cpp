#include "pixlie/parser.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "pixlie/processors/blue_formula.h"
#include "pixlie/processors/green_formula.h"
#include "pixlie/processors/red_formula.h"
#include "pixlie/processors/rotate.h"
#include "pixlie/utils/logging.h"

namespace {

std::string_view trim(std::string_view value) {
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

std::vector<std::string> split_words(std::string_view value) {
    std::istringstream stream{std::string(value)};
    std::vector<std::string> words;
    for (std::string word; stream >> word;) {
        words.push_back(std::move(word));
    }
    return words;
}

std::optional<ProcessorCommand> invalid(
    std::string_view command,
    std::string_view reason
) {
    log(
        LogLevel::error,
        "Invalid processor \"" + std::string(command) + "\": " + std::string(reason)
    );
    return std::nullopt;
}

} // namespace

std::optional<ProcessorCommand> parse_processor_command(std::string_view command) {
    const std::string_view value = trim(command);
    if (value.empty()) {
        return invalid(command, "command is empty");
    }

    const ImageProcessor* processor = nullptr;
    std::vector<std::string> arguments;

    const auto words = split_words(value);
    if (!words.empty() && words.front() == "rotate") {
        processor = &rotate_processor();
        arguments.assign(words.begin() + 1, words.end());
    } else {
        const std::size_t equals = value.find('=');
        if (equals == std::string_view::npos) {
            return invalid(command, "unknown processor");
        }

        const std::string_view channel = trim(value.substr(0, equals));
        if (channel == "r") {
            processor = &red_formula_processor();
        } else if (channel == "g") {
            processor = &green_formula_processor();
        } else if (channel == "b") {
            processor = &blue_formula_processor();
        } else {
            return invalid(command, "unknown processor");
        }

        const std::string_view formula = trim(value.substr(equals + 1));
        arguments.emplace_back(formula);
    }

    try {
        processor->validate(arguments);
    } catch (const std::exception& error) {
        return invalid(command, error.what());
    }

    return ProcessorCommand{
        std::cref(*processor),
        std::move(arguments),
        std::string(command),
    };
}
