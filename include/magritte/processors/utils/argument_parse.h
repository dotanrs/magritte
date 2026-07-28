#ifndef MAGRITTE_PROCESSOR_ARGUMENT_PARSE_H
#define MAGRITTE_PROCESSOR_ARGUMENT_PARSE_H

#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

    /// Recognizes `<keyword> [arguments...]` and returns the words following
    /// the keyword, or `std::nullopt` when the first word does not match.
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

    /// Recognizes `<keyword> = <value>` and returns the trimmed right-hand side
    /// as one argument, preserving internal whitespace.
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

    /// Recognizes `<keyword> <count> = <value>` and returns the count followed
    /// by the trimmed right-hand side.
    inline std::optional<std::vector<std::string>> after_counted_assignment(
        std::string_view command,
        std::string_view keyword
    ) {
        const std::string_view value = trim(command);
        const auto words = split_words(value);
        if (words.empty() || words.front() != keyword) {
            return std::nullopt;
        }

        const std::size_t equals = value.find('=');
        if (equals == std::string_view::npos) {
            throw std::invalid_argument(
                std::string(keyword) + " expects '<iterations> = <value>'"
            );
        }

        const auto left_words = split_words(trim(value.substr(0, equals)));
        if (left_words.size() != 2) {
            throw std::invalid_argument(
                std::string(keyword) +
                " expects one iteration count before '='"
            );
        }

        return std::vector<std::string>{
            left_words[1],
            std::string(trim(value.substr(equals + 1)))
        };
    }

    /// Recognizes `<left> <keyword> <right...>` and returns all words except
    /// the infix keyword.
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

#endif //MAGRITTE_PROCESSOR_ARGUMENT_PARSE_H
