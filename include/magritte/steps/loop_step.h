#ifndef MAGRITTE_LOOP_STEP_H
#define MAGRITTE_LOOP_STEP_H

#include <functional>
#include <string>
#include <vector>

#include "magritte/steps/image_step.h"

/// Repeatedly applies another step. The first argument is the iteration
/// count; all remaining arguments are passed to the wrapped step.
class LoopStep : public ImageStep {
public:
    explicit LoopStep(const ImageStep &substep);

    void validate(const std::vector<std::string> &arguments) const final;

    [[nodiscard]] FileData apply(
        FileData data,
        const std::vector<std::string> &arguments,
        const MacroMap *macros = nullptr
    ) const final;

protected:
    [[nodiscard]] const ImageStep &substep() const noexcept;

private:
    std::reference_wrapper<const ImageStep> substep_;
};

#endif // MAGRITTE_LOOP_STEP_H
