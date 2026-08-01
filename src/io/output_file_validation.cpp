//
// Created by Dotan Reis on 01/08/2026.
//

#include "magritte/io/output_file_validation.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "magritte/utils/logging.h"

namespace fs = std::filesystem;

bool confirm_overwrite(const fs::path &output) {
    std::cerr << "Output file already exists: " << output.string() << '\n'
            << "Continue and overwrite it? [y/N] " << std::flush;

    std::string response;
    if (!std::getline(std::cin, response)) {
        std::cerr << '\n';
        return false;
    }

    response.erase(
        response.begin(),
        std::ranges::find_if(response
                             ,
                             [](unsigned char character) {
                                 return !std::isspace(character);
                             }
        )
    );
    response.erase(
        std::find_if(
            response.rbegin(),
            response.rend(),
            [](unsigned char character) {
                return !std::isspace(character);
            }
        ).base(),
        response.end()
    );
    std::ranges::transform(response
                           ,
                           response.begin(),
                           [](unsigned char character) {
                               return static_cast<char>(std::tolower(character));
                           }
    );
    return response == "y" || response == "yes";
}

bool should_write_output(const fs::path &output, bool overwrite) {
    std::error_code error;
    const bool output_exists = fs::exists(output, error);
    if (error) {
        throw std::runtime_error(
            "could not inspect output path: " + error.message()
        );
    }
    return !output_exists || overwrite || confirm_overwrite(output);
}
