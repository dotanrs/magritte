#ifndef MAGRITTE_FILE_UTILS_H
#define MAGRITTE_FILE_UTILS_H

#include <filesystem>

#include "magritte/common/file_data.h"

/// Decodes a JPEG and applies its orientation metadata.
/// @throws std::runtime_error if the file cannot be read or decoded.
[[nodiscard]] FileData read_file(const std::filesystem::path &input);

/// Encodes `data` as JPEG, creating missing parent directories and replacing
/// any existing destination file.
/// @throws std::runtime_error if the buffer is invalid or encoding/writing fails.
void save_file(const std::filesystem::path &output, const FileData &data);

/// Verifies that the dimensions are nonzero, their product does not overflow,
/// and the pixel count equals `width * height`.
/// @throws std::runtime_error when the image buffer violates the invariant.
void validate_file_data(const FileData &data);

#endif // MAGRITTE_FILE_UTILS_H
