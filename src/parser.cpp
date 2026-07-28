#include "magritte/parser.h"

#include <array>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string>
#include "magritte/utils/string.h"
#include "magritte/processors/black_and_white.h"
#include "magritte/processors/blur.h"
#include "magritte/processors/contrast.h"
#include "magritte/processors/fisheye.h"
#include "magritte/processors/flow_lines.h"
#include "magritte/processors/lighting.h"
#include "magritte/processors/local_rgb.h"
#include "magritte/processors/local_warp.h"
#include "magritte/processors/loop_rgb.h"
#include "magritte/processors/loop_warp.h"
#include "magritte/processors/mirror.h"
#include "magritte/processors/rgb_formula.h"
#include "magritte/processors/rotate.h"
#include "magritte/processors/saturation_formula.h"
#include "magritte/processors/spin.h"
#include "magritte/processors/twist.h"
#include "magritte/processors/warp_formula.h"

namespace {
    std::optional<ProcessorCommand> invalid(
        std::string_view reason,
        std::string *error_message
    ) {
        if (error_message != nullptr) {
            *error_message = reason;
        }
        return std::nullopt;
    }

    const std::array<std::reference_wrapper<const ImageProcessor>, 17>
    processors{
        rotate_processor(),
        mirror_processor(),
        blur_processor(),
        black_and_white_processor(),
        contrast_processor(),
        fisheye_processor(),
        twist_processor(),
        spin_processor(),
        flow_lines_processor(),
        lighting_processor(),
        rgb_formula_processor(),
        local_rgb_processor(),
        local_warp_processor(),
        saturation_formula_processor(),
        loop_rgb_processor(),
        loop_warp_processor(),
        warp_formula_processor(),
    };
} // namespace

std::optional<ProcessorCommand> parse_processor_command(
    std::string_view command,
    std::string *error_message
) {
    const std::string_view value = trim(command);
    if (value.empty()) {
        return invalid("command is empty", error_message);
    }

    for (const ImageProcessor &processor: processors) {
        try {
            if (auto arguments = processor.parse_arguments(value)) {
                return ProcessorCommand{
                    std::cref(processor),
                    std::move(*arguments),
                    std::string(command),
                };
            }
        } catch (const std::exception &error) {
            return invalid(error.what(), error_message);
        }
    }

    return invalid("unknown processor", error_message);
}
