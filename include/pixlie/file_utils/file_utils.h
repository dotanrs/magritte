#ifndef PIXLIE_FILE_UTILS_H
#define PIXLIE_FILE_UTILS_H

#include <filesystem>

#include "pixlie/file_utils/file_data.h"

[[nodiscard]] FileData read_file(const std::filesystem::path &input);

void save_file(const std::filesystem::path &output, const FileData &data);

void validate_file_data(const FileData &data);

#endif // PIXLIE_FILE_UTILS_H
