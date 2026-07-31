#ifndef MAGRITTE_IMAGE_PROCESSOR_H
#define MAGRITTE_IMAGE_PROCESSOR_H

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "magritte/macro.h"
#include "magritte/processor.h"

/// Interface for recognizing, validating, and applying one kind of image
/// processor command.
class ImageProcessor {
public:
    virtual ~ImageProcessor() = default;

    /// Returns the human-readable processor name used in progress logs.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Recognizes `command` and returns its validated arguments.
    ///
    /// A non-matching command returns `std::nullopt`; a matching but malformed
    /// command throws `std::invalid_argument`.
    [[nodiscard]] virtual std::optional<std::vector<std::string>>
    parse_arguments(std::string_view command) const = 0;

    /// Checks arguments independently of command recognition.
    /// @throws std::invalid_argument when the arguments are not supported.
    virtual void validate(const std::vector<std::string> &arguments) const = 0;

    /// Returns the transformed image. Implementations may reuse the input
    /// buffer because it is passed by value. Formula-aware processors may
    /// resolve variables from the optional global macro map.
    [[nodiscard]] virtual FileData apply(
        FileData data,
        const std::vector<std::string> &arguments,
        const MacroMap *macros = nullptr
    ) const = 0;

    /// Adds visual debugging hints for this processor to an already transformed
    /// image. Processors without debug visualization return the image unchanged.
    [[nodiscard]] virtual FileData add_debug_hints(
        FileData data,
        const std::vector<std::string> &
    ) const {
        return data;
    }
};

struct ProcessorCommand {
    std::reference_wrapper<const ImageProcessor> processor;
    std::vector<std::string> arguments;
    std::string original_command;
};

#endif //MAGRITTE_IMAGE_PROCESSOR_H
