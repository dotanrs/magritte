#include "../../include/magritte/input_parsing/step_comman_parser.h"

#include <array>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string>
#include "magritte/utils/string.h"
#include "magritte/steps/black_and_white.h"
#include "magritte/steps/blur.h"
#include "magritte/steps/contrast.h"
#include "magritte/steps/fisheye.h"
#include "magritte/steps/flow_lines.h"
#include "magritte/steps/lighting.h"
#include "magritte/steps/local_rgb.h"
#include "magritte/steps/local_warp.h"
#include "magritte/steps/loop_rgb.h"
#include "magritte/steps/loop_warp.h"
#include "magritte/steps/mirror.h"
#include "magritte/steps/rgb_formula.h"
#include "magritte/steps/rotate.h"
#include "magritte/steps/saturation_formula.h"
#include "magritte/steps/spin.h"
#include "magritte/steps/twist.h"
#include "magritte/steps/warp_formula.h"

namespace {
    std::optional<StepCommand> invalid(
        std::string_view reason,
        std::string *error_message
    ) {
        if (error_message != nullptr) {
            *error_message = reason;
        }
        return std::nullopt;
    }

    const std::array<std::reference_wrapper<const ImageStep>, 17> &
    registered_steps() {
        static const std::array<
            std::reference_wrapper<const ImageStep>,
            17
        > steps{
            rotate_step(),
            mirror_step(),
            blur_step(),
            black_and_white_step(),
            contrast_step(),
            fisheye_step(),
            twist_step(),
            spin_step(),
            flow_lines_step(),
            lighting_step(),
            rgb_formula_step(),
            local_rgb_step(),
            local_warp_step(),
            saturation_formula_step(),
            loop_rgb_step(),
            loop_warp_step(),
            warp_formula_step(),
        };
        return steps;
    }
} // namespace

std::optional<StepCommand> parse_step_command(
    std::string_view command,
    std::string *error_message
) {
    const std::string_view value = trim(command);
    if (value.empty()) {
        return invalid("command is empty", error_message);
    }

    for (const ImageStep &step: registered_steps()) {
        try {
            if (auto arguments = step.parse_arguments(value)) {
                return StepCommand{
                    std::cref(step),
                    std::move(*arguments),
                    std::string(command),
                };
            }
        } catch (const std::exception &error) {
            return invalid(error.what(), error_message);
        }
    }

    return invalid("unknown step", error_message);
}
