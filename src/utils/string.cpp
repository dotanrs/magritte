//
// Created by Dotan Reis on 27/07/2026.
//

#include "magritte/utils/string.h"
#include <string>

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