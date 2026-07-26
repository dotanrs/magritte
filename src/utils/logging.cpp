//
// Created by Dotan Reis on 26/07/2026.
//

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include "../../include/utils/logging.h"

namespace {
    std::string timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto milliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm utc_time{};
#ifdef _WIN32
        gmtime_s(&utc_time, &time);
#else
        gmtime_r(&time, &utc_time);
#endif

        std::ostringstream result;
        result << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << milliseconds.count() << 'Z';
        return result.str();
    }
}

void log(LogLevel level, std::string_view message) {
    std::ostream &output = level == LogLevel::error ? std::cerr : std::clog;
    output << timestamp() << " [" << (level == LogLevel::error ? "ERROR" : "INFO")
            << "] " << message << '\n';
}
