//
// Created by Dotan Reis on 26/07/2026.
//

#ifndef PIXLIE_INPUTVALIDATION_H
#define PIXLIE_INPUTVALIDATION_H

#include "processor.h"

std::tuple<fs::path, fs::path> validate_input(const Options& options);

#endif //PIXLIE_INPUTVALIDATION_H
