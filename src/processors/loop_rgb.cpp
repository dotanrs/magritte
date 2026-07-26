#include "pixlie/processors/loop_rgb.h"

#include "pixlie/processors/loop_assignment_processor.h"
#include "pixlie/processors/rgb_formula.h"

const ImageProcessor &loop_rgb_processor() {
    static const LoopAssignmentProcessor processor{
        "rgb",
        rgb_formula_processor()
    };
    return processor;
}
