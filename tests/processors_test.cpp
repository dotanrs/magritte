#include <iostream>

#include "common/test_support.h"

void test_rotation();
void test_mirror();
void test_blur();
void test_lighting();
void test_red_formula_and_clamping();
void test_green_formula();
void test_blue_formula();
void test_formula_coordinates_and_dimensions();
void test_formula_normalized_and_polar_coordinates();
void test_formula_math_functions();
void test_simultaneous_rgb_formula();
void test_local_rgb_formula();
void test_local_warp_formula();
void test_warp_formula();
void test_loop_rgb();
void test_loop_warp();
void test_fisheye();
void test_fisheye_debug_hints();
void test_twist();
void test_flow_lines();
void test_saturation_formula();
void test_rgb_formula_target_order();
void test_processor_argument_parsing();
void test_command_parser();
void test_drawing_config();

int main() {
    test_rotation();
    test_mirror();
    test_blur();
    test_lighting();
    test_red_formula_and_clamping();
    test_green_formula();
    test_blue_formula();
    test_formula_coordinates_and_dimensions();
    test_formula_normalized_and_polar_coordinates();
    test_formula_math_functions();
    test_simultaneous_rgb_formula();
    test_local_rgb_formula();
    test_local_warp_formula();
    test_warp_formula();
    test_loop_rgb();
    test_loop_warp();
    test_fisheye();
    test_fisheye_debug_hints();
    test_twist();
    test_flow_lines();
    test_saturation_formula();
    test_rgb_formula_target_order();
    test_processor_argument_parsing();
    test_command_parser();
    test_drawing_config();

    if (failures == 0) {
        std::cout << "All processor tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
