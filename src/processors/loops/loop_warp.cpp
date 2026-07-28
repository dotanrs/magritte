// Processor: `loop-warp <iterations> = (<source-x>, <source-y>)`.
// Repeatedly applies a coordinate warp, feeding each result into the next
// pass. `iterations` is a nonnegative integer; `source-x` and `source-y` are
// formulas for the input coordinates sampled during each pass.

#include "magritte/processors/loop_warp.h"

#include "magritte/processors/loop_assignment_processor.h"
#include "magritte/processors/warp_formula.h"

const ImageProcessor &loop_warp_processor() {
    static const LoopAssignmentProcessor processor{
        "warp",
        warp_formula_processor()
    };
    return processor;
}
