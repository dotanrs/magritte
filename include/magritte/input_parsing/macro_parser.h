#ifndef MAGRITTE_MACRO_H
#define MAGRITTE_MACRO_H

#include <map>
#include <string>
#include <string_view>
#include <utility>

using MacroMap = std::map<std::string, std::string>;

/// Parses `macro_<name>=<formula>`, canonicalizing the name to lowercase.
/// Macro names must use formula-identifier characters and values cannot be
/// empty.
/// @throws std::invalid_argument for malformed definitions.
[[nodiscard]] std::pair<std::string, std::string> parse_macro_definition(
    std::string_view definition
);

/// Adds a macro. An identical existing definition is accepted; a different
/// value for the same canonical name is a conflict.
/// @throws std::invalid_argument for conflicting definitions.
void add_macro(
    MacroMap &macros,
    std::string name,
    std::string value
);

/// Merges all definitions using the same conflict rules as `add_macro`.
void merge_macros(MacroMap &destination, const MacroMap &source);

#endif // MAGRITTE_MACRO_H
