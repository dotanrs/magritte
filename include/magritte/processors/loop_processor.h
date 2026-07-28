#ifndef MAGRITTE_LOOP_PROCESSOR_H
#define MAGRITTE_LOOP_PROCESSOR_H

#include <functional>
#include <string>
#include <vector>

#include "magritte/processors/image_processor.h"

/// Repeatedly applies another processor. The first argument is the iteration
/// count; all remaining arguments are passed to the wrapped processor.
class LoopProcessor : public ImageProcessor {
public:
    explicit LoopProcessor(const ImageProcessor &subprocessor);

    void validate(const std::vector<std::string> &arguments) const final;

    [[nodiscard]] FileData apply(
        FileData data,
        const std::vector<std::string> &arguments
    ) const final;

protected:
    [[nodiscard]] const ImageProcessor &subprocessor() const noexcept;

private:
    std::reference_wrapper<const ImageProcessor> subprocessor_;
};

#endif // MAGRITTE_LOOP_PROCESSOR_H
