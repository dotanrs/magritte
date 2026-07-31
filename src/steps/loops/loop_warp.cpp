// Step: `loop-warp <iterations> = (<source-x>, <source-y>)`.
// Repeatedly applies a coordinate warp, feeding each result into the next
// pass. `iterations` is a nonnegative integer; `source-x` and `source-y` are
// formulas for the input coordinates sampled during each pass.

#include "magritte/steps/loop_warp.h"

#include "magritte/steps/loop_assignment_step.h"
#include "magritte/steps/warp_formula.h"

const ImageStep &loop_warp_step() {
    static const LoopAssignmentStep step{
        "warp",
        warp_formula_step()
    };
    return step;
}
