#include "pixlie/parser.h"

#include <array>
#include <cctype>
#include <functional>
#include <stdexcept>
#include <string>
#include "pixlie/utils/string.h"
#include "pixlie/processors/black_and_white.h"
#include "pixlie/processors/blur.h"
#include "pixlie/processors/contrast.h"
#include "pixlie/processors/fisheye.h"
#include "pixlie/processors/flow_lines.h"
#include "pixlie/processors/lighting.h"
#include "pixlie/processors/local_rgb.h"
#include "pixlie/processors/local_warp.h"
#include "pixlie/processors/loop_rgb.h"
#include "pixlie/processors/loop_warp.h"
#include "pixlie/processors/mirror.h"
#include "pixlie/processors/rgb_formula.h"
#include "pixlie/processors/rotate.h"
#include "pixlie/processors/saturation_formula.h"
#include "pixlie/processors/spin.h"
#include "pixlie/processors/twist.h"
#include "pixlie/processors/warp_formula.h"

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
