#ifndef PIXLIE_PROCESSOR_ARGUMENT_PARSE_H
#define PIXLIE_PROCESSOR_ARGUMENT_PARSE_H

#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace processor_argument_parse {
    inline std::string_view trim(std::string_view value) {
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

    inline std::vector<std::string> split_words(std::string_view value) {
        std::istringstream stream{std::string(value)};
        std::vector<std::string> words;
        for (std::string word; stream >> word;) {
            words.push_back(std::move(word));
        }
        return words;
    }

    inline std::optional<std::vector<std::string>> after_keyword(
        std::string_view command,
        std::string_view keyword
    ) {
        auto words = split_words(command);
        if (words.empty() || words.front() != keyword) {
            return std::nullopt;
        }

        return std::vector<std::string>(words.begin() + 1, words.end());
    }

    inline std::optional<std::vector<std::string>> after_assignment(
        std::string_view command,
        std::string_view keyword
    ) {
        const std::string_view value = trim(command);
        const std::size_t equals = value.find('=');
        if (equals == std::string_view::npos ||
            trim(value.substr(0, equals)) != keyword) {
            return std::nullopt;
        }

        return std::vector<std::string>{
            std::string(trim(value.substr(equals + 1)))
        };
    }

    inline std::optional<std::vector<std::string>> around_keyword(
        std::string_view command,
        std::string_view keyword
    ) {
        auto words = split_words(command);
        if (words.size() < 2 || words[1] != keyword) {
            return std::nullopt;
        }

        words.erase(words.begin() + 1);
        return words;
    }
} // namespace processor_argument_parse

#endif //PIXLIE_PROCESSOR_ARGUMENT_PARSE_H
