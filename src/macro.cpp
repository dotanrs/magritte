#include "magritte/macro.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

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
}

std::pair<std::string, std::string> parse_macro_definition(
    std::string_view definition
) {
    definition = trim(definition);
    const std::size_t equals = definition.find('=');
    if (equals == std::string_view::npos) {
        throw std::invalid_argument(
            "macro must use the form macro_<name>=<formula>"
        );
    }

    std::string name(trim(definition.substr(0, equals)));
    const std::string_view value = trim(definition.substr(equals + 1));
    if (!name.starts_with("macro_")) {
        throw std::invalid_argument(
            "macro name must explicitly start with 'macro_'"
        );
    }
    if (name.size() == std::string_view("macro_").size()) {
        throw std::invalid_argument("macro name cannot be empty");
    }
    if (!std::all_of(
            name.begin(),
            name.end(),
            [](unsigned char character) {
                return std::isalnum(character) != 0 || character == '_';
            }
        )) {
        throw std::invalid_argument(
            "macro name may only contain letters, numbers, and underscores"
        );
    }
    if (value.empty()) {
        throw std::invalid_argument("macro formula cannot be empty");
    }

    std::transform(
        name.begin(),
        name.end(),
        name.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );
    return {std::move(name), std::string(value)};
}

void add_macro(
    MacroMap &macros,
    std::string name,
    std::string value
) {
    const auto existing = macros.find(name);
    if (existing != macros.end()) {
        if (existing->second == value) {
            return;
        }
        throw std::invalid_argument(
            "conflicting macro definition for '" + existing->first + "'"
        );
    }
    macros.emplace(std::move(name), std::move(value));
}

void merge_macros(MacroMap &destination, const MacroMap &source) {
    for (const auto &[name, value]: source) {
        add_macro(destination, name, value);
    }
}
