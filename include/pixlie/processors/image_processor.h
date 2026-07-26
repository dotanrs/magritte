#ifndef PIXLIE_IMAGE_PROCESSOR_H
#define PIXLIE_IMAGE_PROCESSOR_H

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "pixlie/processor.h"

class ImageProcessor {
public:
    virtual ~ImageProcessor() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual void validate(const std::vector<std::string> &arguments) const = 0;

    [[nodiscard]] virtual FileData apply(
        FileData data,
        const std::vector<std::string> &arguments
    ) const = 0;
};

struct ProcessorCommand {
    std::reference_wrapper<const ImageProcessor> processor;
    std::vector<std::string> arguments;
    std::string source;
};

#endif //PIXLIE_IMAGE_PROCESSOR_H
