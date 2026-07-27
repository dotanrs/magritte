#include "pixlie/processors/loop_warp.h"

#include "pixlie/processors/loop_assignment_processor.h"
#include "pixlie/processors/warp_formula.h"

const ImageProcessor &loop_warp_processor() {
    static const LoopAssignmentProcessor processor{
        "warp",
        warp_formula_processor()
    };
    return processor;
}
