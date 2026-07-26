#include "pixlie/parser.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "pixlie/processors/blue_formula.h"
#include "pixlie/processors/blur.h"
#include "pixlie/processors/color_swap.h"
#include "pixlie/processors/green_formula.h"
#include "pixlie/processors/mirror.h"
#include "pixlie/processors/red_formula.h"
#include "pixlie/processors/rgb_formula.h"
#include "pixlie/processors/rotate.h"
#include "pixlie/processors/saturation_formula.h"

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
        std::string_view reason,
        std::string *error_message
    ) {
        if (error_message != nullptr) {
            *error_message = reason;
        }
        return std::nullopt;
    }
} // namespace

std::optional<ProcessorCommand> parse_processor_command(
    std::string_view command,
    std::string *error_message
) {
    const std::string_view value = trim(command);
    if (value.empty()) {
        return invalid("command is empty", error_message);
    }

    const ImageProcessor *processor = nullptr;
    std::vector<std::string> arguments;

    const auto words = split_words(value);
    if (!words.empty() && words.front() == "rotate") {
        processor = &rotate_processor();
        arguments.assign(words.begin() + 1, words.end());
    } else if (!words.empty() && words.front() == "mirror") {
        processor = &mirror_processor();
        arguments.assign(words.begin() + 1, words.end());
    } else if (!words.empty() && words.front() == "blur") {
        processor = &blur_processor();
        arguments.assign(words.begin() + 1, words.end());
    } else if (words.size() == 3 && words[1] == "<->") {
        processor = &color_swap_processor();
        arguments = {words[0], words[2]};
    } else {
        const std::size_t equals = value.find('=');
        if (equals == std::string_view::npos) {
            return invalid("unknown processor", error_message);
        }

        const std::string_view channel = trim(value.substr(0, equals));
        if (channel == "r") {
            processor = &red_formula_processor();
        } else if (channel == "g") {
            processor = &green_formula_processor();
        } else if (channel == "b") {
            processor = &blue_formula_processor();
        } else if (channel == "rgb") {
            processor = &rgb_formula_processor();
        } else if (channel == "s") {
            processor = &saturation_formula_processor();
        } else {
            return invalid("unknown processor", error_message);
        }

        const std::string_view formula = trim(value.substr(equals + 1));
        arguments.emplace_back(formula);
    }

    try {
        processor->validate(arguments);
    } catch (const std::exception &error) {
        return invalid(error.what(), error_message);
    }

    return ProcessorCommand{
        std::cref(*processor),
        std::move(arguments),
        std::string(command),
    };
}
