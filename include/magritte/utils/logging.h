//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef MAGRITTE_LOGGING_H
#define MAGRITTE_LOGGING_H

#include <string_view>


enum class LogLevel {
    info,
    error,
};

/// Writes a UTC-timestamped message to the info or error stream.
void log(LogLevel level, std::string_view message);

#endif //MAGRITTE_LOGGING_H
