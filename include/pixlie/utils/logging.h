//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_LOGGING_H
#define PIXLIE_LOGGING_H

#include <string_view>


enum class LogLevel {
    info,
    error,
};

/// Writes a UTC-timestamped message to the info or error stream.
void log(LogLevel level, std::string_view message);

#endif //PIXLIE_LOGGING_H
