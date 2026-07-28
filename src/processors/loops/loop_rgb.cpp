// Processor: `loop-rgb <iterations> = (<red>, <green>, <blue>)`.
// Repeatedly applies an RGB formula, feeding each result into the next pass.
// `iterations` is a nonnegative integer; `red`, `green`, and `blue` are the
// formulas for their corresponding channels in each pass.

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
