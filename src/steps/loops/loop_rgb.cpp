// Step: `loop-rgb <iterations> = (<red>, <green>, <blue>)`.
// Repeatedly applies an RGB formula, feeding each result into the next pass.
// `iterations` is a nonnegative integer; `red`, `green`, and `blue` are the
// formulas for their corresponding channels in each pass.

#include "magritte/steps/loop_rgb.h"

#include "magritte/steps/loop_assignment_step.h"
#include "magritte/steps/rgb_formula.h"

const ImageStep &loop_rgb_step() {
    static const LoopAssignmentStep step{
        "rgb",
        rgb_formula_step()
    };
    return step;
}
