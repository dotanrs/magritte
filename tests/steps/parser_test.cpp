#include <optional>
#include "../common/test_support.h"
#include "magritte/input_parsing/step_command_parser.h"
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
#include "magritte/steps/spin.h"
#include "magritte/steps/twist.h"
#include "magritte/steps/warp_formula.h"

void test_step_argument_parsing() {
const auto rotate_arguments =
    rotate_step().parse_arguments("rotate -1");
expect(
    rotate_arguments ==
    std::optional<std::vector<std::string>>{{"-1"}},
    "rotate step should parse its own arguments"
);
expect(
    !rotate_step().parse_arguments("mirror x").has_value(),
    "rotate step should decline another step's command"
);

const auto contrast_arguments =
    contrast_step().parse_arguments("contrast 2");
expect(
    contrast_arguments ==
    std::optional<std::vector<std::string>>{{"2"}},
    "contrast step should parse its factor"
);
expect(
    !contrast_step().parse_arguments("blur 3").has_value(),
    "contrast step should decline another step's command"
);

const auto black_and_white_arguments =
    black_and_white_step().parse_arguments("black-and-white 1.2");
expect(
    black_and_white_arguments ==
    std::optional<std::vector<std::string>>{{"1.2"}},
    "black-and-white step should parse its brightness multiplier"
);
expect(
    !black_and_white_step().parse_arguments("blur 3").has_value(),
    "black-and-white step should decline another step's command"
);

const auto rgb_arguments =
    rgb_formula_step().parse_arguments("rgb = (G, B, R)");
expect(
    rgb_arguments ==
    std::optional<std::vector<std::string>>{
        {"rgb", "(G, B, R)"}
    },
    "RGB step should parse its assignment"
);
const auto offset_rgb_arguments =
    rgb_formula_step().parse_arguments(
        "rgb offset 25 70 30 = (R + A, G + D, B)"
    );
expect(
    offset_rgb_arguments ==
    std::optional<std::vector<std::string>>{
        {"rgb", "(R + A, G + D, B)", "25", "70", "30"}
    },
    "RGB step should parse a percentage origin and radius"
);
const auto subset_rgb_arguments =
    rgb_formula_step().parse_arguments("bgr = (R, G, B)");
expect(
    subset_rgb_arguments ==
    std::optional<std::vector<std::string>>{
        {"bgr", "(R, G, B)"}
    },
    "RGB step should preserve a reordered target"
);
expect(
    rgb_formula_step().parse_arguments("r = G") ==
    std::optional<std::vector<std::string>>{{"r", "G"}},
    "RGB step should parse a single-channel assignment"
);

const auto local_rgb_arguments =
    local_rgb_step().parse_arguments(
        "local-rgb = (red(-1, 0), G, blue(1, 0))"
    );
expect(
    local_rgb_arguments ==
    std::optional<std::vector<std::string>>{
        {"(red(-1, 0), G, blue(1, 0))"}
    },
    "local-rgb should parse its sampling formula"
);
expect(
    !local_rgb_step().parse_arguments(
        "rgb = (R, G, B)"
    ).has_value(),
    "local-rgb should require its own assignment keyword"
);

const auto local_warp_arguments =
    local_warp_step().parse_arguments(
        "local-warp = (X + red(1, 0) / 255, Y)"
    );
expect(
    local_warp_arguments ==
    std::optional<std::vector<std::string>>{
        {"(X + red(1, 0) / 255, Y)"}
    },
    "local-warp should parse its sampling formula"
);

const auto warp_arguments =
    warp_formula_step().parse_arguments(
        "warp = (X + sin(Y), Y)"
    );
expect(
    warp_arguments ==
    std::optional<std::vector<std::string>>{
        {"(X + sin(Y), Y)"}
    },
    "warp step should parse its coordinate assignment"
);
expect(
    !warp_formula_step().parse_arguments(
        "rgb = (R, G, B)"
    ).has_value(),
    "warp step should require its own assignment keyword"
);

const auto loop_warp_arguments =
    loop_warp_step().parse_arguments(
        "loop-warp 3 = (X + sin(Y), Y)"
    );
expect(
    loop_warp_arguments ==
    std::optional<std::vector<std::string>>{
        {"3", "(X + sin(Y), Y)"}
    },
    "loop-warp should parse a count and preserve its warp formula"
);
expect(
    !loop_warp_step().parse_arguments(
        "warp = (X, Y)"
    ).has_value(),
    "loop-warp should require its own assignment keyword"
);

const auto loop_rgb_arguments =
    loop_rgb_step().parse_arguments(
        "loop-rgb 4 = (G, B, R)"
    );
expect(
    loop_rgb_arguments ==
    std::optional<std::vector<std::string>>{
        {"4", "(G, B, R)"}
    },
    "loop-rgb should parse a count and preserve its RGB formula"
);
expect(
    !loop_rgb_step().parse_arguments(
        "rgb = (R, G, B)"
    ).has_value(),
    "loop-rgb should require its own assignment keyword"
);

const auto fisheye_arguments =
    fisheye_step().parse_arguments("fisheye 50 50 1 25");
expect(
    fisheye_arguments ==
    std::optional<std::vector<std::string>>{
        {"50", "50", "1", "25"}
    },
    "fisheye step should parse its center, amount, and radius"
);
expect(
    !fisheye_step().parse_arguments("blur 3").has_value(),
    "fisheye step should decline another step's command"
);

const auto twist_arguments =
    twist_step().parse_arguments("twist 50 40 0.75 20");
expect(
    twist_arguments ==
    std::optional<std::vector<std::string>>{{"50", "40", "0.75", "20"}},
    "twist step should parse its center, force, and radius"
);
expect(
    !twist_step().parse_arguments("blur 3").has_value(),
    "twist step should decline another step's command"
);

const auto spin_arguments =
    spin_step().parse_arguments("spin 50 40 90 20");
expect(
    spin_arguments ==
    std::optional<std::vector<std::string>>{{"50", "40", "90", "20"}},
    "spin step should parse its center, angle, and radius"
);
expect(
    !spin_step().parse_arguments("blur 3").has_value(),
    "spin step should decline another step's command"
);

const auto flow_line_arguments =
    flow_lines_step().parse_arguments(
        "flow-lines 18 600 1.25 1.2 #173F70 0.8 = (-V, U)"
    );
expect(
    flow_line_arguments ==
    std::optional<std::vector<std::string>>{
        {
            "18",
            "600",
            "1.25",
            "1.2",
            "#173F70",
            "0.8",
            "(-V, U)",
        }
    },
    "flow-lines should parse style arguments and its vector equation"
);
expect(
    !flow_lines_step().parse_arguments("blur 3").has_value(),
    "flow-lines should decline another step's command"
);

const auto lighting_arguments =
    lighting_step().parse_arguments(
        "lighting 315 #FFD080 64 0.7"
    );
expect(
    lighting_arguments ==
    std::optional<std::vector<std::string>>{
        {"315", "#FFD080", "64", "0.7"}
    },
    "lighting step should parse angle, color, threshold, and strength"
);
const auto lighting_preset_arguments =
    lighting_step().parse_arguments("lighting studio 0.8");
expect(
    lighting_preset_arguments ==
    std::optional<std::vector<std::string>>{{"studio", "0.8"}},
    "lighting step should parse a preset and its strength"
);
expect(
    !lighting_step().parse_arguments("blur 3").has_value(),
    "lighting step should decline another step's command"
);

}

void test_command_parser() {
std::string error_message;

expect(
    parse_step_command("rotate -1").has_value(),
    "parser should accept a valid rotate command"
);
expect(
    parse_step_command("mirror x").has_value(),
    "parser should accept a valid mirror command"
);
expect(
    parse_step_command("blur 3").has_value(),
    "parser should accept a valid blur command"
);
expect(
    parse_step_command("contrast 2").has_value(),
    "parser should accept a contrast command"
);
expect(
    parse_step_command("black-and-white 1.2").has_value(),
    "parser should accept a black-and-white command"
);
expect(
    parse_step_command("fisheye 50 50 -0.5").has_value(),
    "parser should accept a fisheye command with default radius"
);
expect(
    parse_step_command("fisheye 50 50 -0.5 25").has_value(),
    "parser should accept a fisheye command with explicit radius"
);
expect(
    parse_step_command("twist 50 50 0.5").has_value(),
    "parser should accept a twist command"
);
expect(
    parse_step_command("spin 50 50 45 25").has_value(),
    "parser should accept a spin command"
);
expect(
    parse_step_command(
        "lighting 315 #FFD080 64 0.7"
    ).has_value(),
    "parser should accept a lighting command"
);
expect(
    parse_step_command("lighting synthwave").has_value(),
    "parser should accept a lighting preset"
);
expect(
    parse_step_command(
        "lighting 315 #FFD080 auto 0.7 12 0.1"
    ).has_value(),
    "parser should accept automatic threshold, softness, and atmosphere"
);
expect(
    parse_step_command("r = (R + G) / 2").has_value(),
    "parser should accept a valid red formula"
);
expect(
    parse_step_command(
        "r = 127 + 127 * sin(X / 12 + A)"
    ).has_value(),
    "parser should accept coordinate-aware mathematical formulas"
);
expect(
    parse_step_command(
        "rgb = (G, B, R)"
    ).has_value(),
    "parser should accept a simultaneous RGB formula"
);
expect(
    parse_step_command(
        "rgb offset 25 70 30 = (R + A, G + D, B)"
    ).has_value(),
    "parser should accept a localized RGB formula"
);
expect(
    parse_step_command(
        "rgb = (max(R, G), min(G, B), clamp(B, 0, 255))"
    ).has_value(),
    "RGB tuple separators should coexist with function arguments"
);
expect(
    parse_step_command(
        "bgr = (R, G, B)"
    ).has_value(),
    "parser should accept RGB target channels in any order"
);
expect(
    parse_step_command(
        "rg = (G, R)"
    ).has_value(),
    "parser should accept a subset of RGB target channels"
);
expect(
    parse_step_command(
        "local-rgb = (red(-1, 0), green(0, 1), blue(1, 0))"
    ).has_value(),
    "parser should accept a local RGB sampling formula"
);
expect(
    parse_step_command(
        "loop-rgb 5 = (G, B, R)"
    ).has_value(),
    "parser should accept a counted loop-rgb formula"
);
expect(
    parse_step_command(
        "warp = (X + sin(Y), clamp(Y, 0, H - 1))"
    ).has_value(),
    "parser should accept a mathematical warp formula"
);
expect(
    parse_step_command(
        "loop-warp 3 = (X + sin(Y), clamp(Y, 0, H - 1))"
    ).has_value(),
    "parser should accept a counted loop-warp formula"
);
expect(
    parse_step_command(
        "flow-lines 18 600 1.25 1.2 #173F70 0.8 = (-V, U)"
    ).has_value(),
    "parser should accept a flow-lines vector field"
);
expect(
    parse_step_command("g = B * 2").has_value(),
    "parser should accept a valid green formula"
);
expect(
    parse_step_command("b = R - G").has_value(),
    "parser should accept a valid blue formula"
);
expect(
    parse_step_command("s = S * 2").has_value(),
    "parser should accept a valid saturation formula"
);
expect(
    !parse_step_command("rr = (R, R)", &error_message).has_value(),
    "parser should reject a repeated RGB target channel"
);
expect(
    error_message == "RGB formula target cannot repeat a channel",
    "parser should describe a repeated RGB target channel"
);
error_message.clear();
expect(
    !parse_step_command("mirror z", &error_message).has_value(),
    "parser should reject an invalid mirror axis"
);
expect(
    error_message == "mirror expects exactly one axis: x or y",
    "parser should describe an invalid mirror axis"
);
error_message.clear();
expect(
    !parse_step_command("blur -1", &error_message).has_value(),
    "parser should reject a negative blur radius"
);
expect(
    error_message == "blur radius must be a nonnegative integer",
    "parser should describe an invalid blur radius"
);
error_message.clear();
expect(
    !parse_step_command(
        "black-and-white -0.1",
        &error_message
    ).has_value(),
    "parser should reject a negative black-and-white brightness multiplier"
);
expect(
    error_message ==
    "black-and-white brightness multiplier must be nonnegative",
    "parser should describe an invalid black-and-white brightness multiplier"
);
error_message.clear();
expect(
    !parse_step_command(
        "fisheye 100 75 -1",
        &error_message
    ).has_value(),
    "parser should reject a singular fisheye amount"
);
expect(
    error_message == "fisheye amount must be greater than -1",
    "parser should describe an invalid fisheye amount"
);
error_message.clear();
expect(
    !parse_step_command(
        "fisheye 101 50 1",
        &error_message
    ).has_value(),
    "parser should reject an out-of-range fisheye percentage"
);
expect(
    error_message ==
    "fisheye x and y must be percentages from 0 to 100",
    "parser should describe invalid fisheye percentages"
);
error_message.clear();
expect(
    !parse_step_command(
        "fisheye 100 75",
        &error_message
    ).has_value(),
    "parser should require all three fisheye arguments"
);
expect(
    error_message ==
    "fisheye expects three or four numbers: x y amount [radius]",
    "parser should describe missing fisheye arguments"
);
error_message.clear();
expect(
    !parse_step_command(
        "fisheye 50 50 1 0",
        &error_message
    ).has_value(),
    "parser should reject a nonpositive fisheye radius"
);
expect(
    error_message == "fisheye radius must be greater than 0",
    "parser should describe an invalid fisheye radius"
);
error_message.clear();
expect(
    !parse_step_command("twist 101 50 1", &error_message).has_value(),
    "parser should reject an out-of-range twist center"
);
expect(
    error_message == "twist x and y must be percentages from 0 to 100",
    "parser should describe invalid twist percentages"
);
error_message.clear();
expect(
    !parse_step_command("twist 50 50", &error_message).has_value(),
    "parser should require all three twist arguments"
);
expect(
    error_message ==
    "twist expects three or four numbers: x y force [radius]",
    "parser should describe missing twist arguments"
);
error_message.clear();
expect(
    !parse_step_command("twist 50 50 1 0", &error_message).has_value(),
    "parser should reject a nonpositive twist radius"
);
expect(
    error_message == "twist radius must be greater than 0",
    "parser should describe an invalid twist radius"
);
error_message.clear();
expect(
    !parse_step_command("spin 101 50 90", &error_message).has_value(),
    "parser should reject an out-of-range spin center"
);
expect(
    error_message == "spin x and y must be percentages from 0 to 100",
    "parser should describe invalid spin percentages"
);
error_message.clear();
expect(
    !parse_step_command("spin 50 50", &error_message).has_value(),
    "parser should require all three spin arguments"
);
expect(
    error_message ==
    "spin expects three or four numbers: x y angle [radius]",
    "parser should describe missing spin arguments"
);
error_message.clear();
expect(
    !parse_step_command("spin 50 50 90 0", &error_message).has_value(),
    "parser should reject a nonpositive spin radius"
);
expect(
    error_message == "spin radius must be greater than 0",
    "parser should describe an invalid spin radius"
);
error_message.clear();
expect(
    !parse_step_command(
        "lighting 45 orange 64",
        &error_message
    ).has_value(),
    "parser should reject a non-hex lighting color"
);
expect(
    error_message == "lighting color must use the form #RRGGBB",
    "parser should describe an invalid lighting color"
);
error_message.clear();
expect(
    !parse_step_command(
        "lighting 45 #FFD080 256",
        &error_message
    ).has_value(),
    "parser should reject an out-of-range lighting threshold"
);
expect(
    error_message ==
    "lighting threshold must be an integer from 0 to 255",
    "parser should describe an invalid lighting threshold"
);
error_message.clear();
expect(
    !parse_step_command(
        "lighting 45 #FFD080 64 1.1",
        &error_message
    ).has_value(),
    "parser should reject an out-of-range lighting strength"
);
expect(
    error_message == "lighting strength must be from 0 to 1",
    "parser should describe an invalid lighting strength"
);
error_message.clear();
expect(
    !parse_step_command(
        "lighting 45 #FFD080 auto 0.8 51",
        &error_message
    ).has_value(),
    "parser should reject out-of-range lighting softness"
);
expect(
    error_message ==
    "lighting softness must be from 0 to 50 percent",
    "parser should describe invalid lighting softness"
);
error_message.clear();
expect(
    !parse_step_command(
        "lighting vaporwave",
        &error_message
    ).has_value(),
    "parser should reject unknown lighting presets"
);
expect(
    error_message.find("golden-hour") != std::string::npos,
    "parser should list the available lighting presets"
);
error_message.clear();
expect(
    !parse_step_command("rotate nope", &error_message).has_value(),
    "parser should reject a non-integer rotation"
);
expect(
    !error_message.empty(),
    "parser should return an error message for an invalid command"
);
error_message.clear();
expect(
    !parse_step_command("contrast 0.5", &error_message).has_value(),
    "parser should reject a contrast factor below one"
);
expect(
    error_message == "contrast factor must be at least 1",
    "parser should describe an invalid contrast factor"
);
error_message.clear();
expect(
    !parse_step_command(
        "r = mystery(X)",
        &error_message
    ).has_value(),
    "parser should reject unknown mathematical functions"
);
expect(
    error_message.find("unknown function") != std::string::npos,
    "parser should describe an unknown mathematical function"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb offset 25 = (R, G, B)",
        &error_message
    ).has_value(),
    "parser should require both RGB offset coordinates"
);
expect(
    error_message ==
        "RGB formula offset expects two or three numbers: x y [radius]",
    "parser should describe a missing RGB offset coordinate"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb offset left 70 = (R, G, B)",
        &error_message
    ).has_value(),
    "parser should reject a nonnumeric RGB offset"
);
expect(
    error_message == "RGB formula offset x must be a finite number",
    "parser should describe a nonnumeric RGB offset"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb offset 101 70 = (R, G, B)",
        &error_message
    ).has_value(),
    "parser should reject an out-of-range RGB offset"
);
expect(
    error_message ==
        "RGB formula offset x and y must be percentages from 0 to 100",
    "parser should describe invalid RGB offset percentages"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb offset 25 70 0 = (R, G, B)",
        &error_message
    ).has_value(),
    "parser should reject a nonpositive RGB radius"
);
expect(
    error_message == "RGB formula offset radius must be greater than 0",
    "parser should describe an invalid RGB radius"
);
error_message.clear();
expect(
    !parse_step_command("r = pow(2)", &error_message).has_value(),
    "parser should reject a function with the wrong arity"
);
expect(
    error_message.find("expects 2 arguments") != std::string::npos,
    "parser should describe incorrect function arity"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb = (red(1, 0), G, B)",
        &error_message
    ).has_value(),
    "ordinary RGB formulas should reject local sampling functions"
);
expect(
    error_message.find("only available in local-rgb and local-warp") !=
    std::string::npos,
    "parser should direct sampling functions to local steps"
);
error_message.clear();
expect(
    !parse_step_command(
        "local-rgb = (red(1), G, B)",
        &error_message
    ).has_value(),
    "local-rgb should require two sampling offsets"
);
expect(
    error_message.find("expects 2 arguments") != std::string::npos,
    "parser should describe incorrect sampler arity"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb = R, G, B",
        &error_message
    ).has_value(),
    "parser should require parentheses around an RGB tuple"
);
expect(
    error_message.find("parenthesized tuple") != std::string::npos,
    "parser should describe a missing RGB tuple"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb = (R, G)",
        &error_message
    ).has_value(),
    "parser should require exactly three RGB expressions"
);
expect(
    error_message.find("value count") != std::string::npos,
    "parser should describe an incomplete RGB tuple"
);
error_message.clear();
expect(
    !parse_step_command(
        "rg = (R, G, B)",
        &error_message
    ).has_value(),
    "parser should reject more values than target channels"
);
expect(
    error_message.find("value count") != std::string::npos,
    "parser should describe a target and value count mismatch"
);
error_message.clear();
expect(
    !parse_step_command(
        "rg = (R)",
        &error_message
    ).has_value(),
    "parser should reject fewer values than target channels"
);
expect(
    error_message.find("value count") != std::string::npos,
    "parser should describe a target and value count mismatch"
);
error_message.clear();
expect(
    !parse_step_command(
        "rgb = (R, G, B, 0)",
        &error_message
    ).has_value(),
    "parser should reject more than three RGB expressions"
);
expect(
    error_message.find("value count") != std::string::npos,
    "parser should describe an oversized RGB tuple"
);
error_message.clear();
expect(
    !parse_step_command(
        "warp = X, Y",
        &error_message
    ).has_value(),
    "parser should require parentheses around warp coordinates"
);
expect(
    error_message.find("parenthesized coordinate pair") !=
    std::string::npos,
    "parser should describe a missing warp coordinate pair"
);
error_message.clear();
expect(
    !parse_step_command(
        "warp = (X)",
        &error_message
    ).has_value(),
    "parser should require two warp expressions"
);
expect(
    !error_message.empty(),
    "parser should describe an incomplete warp formula"
);
error_message.clear();
expect(
    !parse_step_command(
        "warp = (X, Y, 0)",
        &error_message
    ).has_value(),
    "parser should reject more than two warp expressions"
);
expect(
    error_message.find("expected ')'") != std::string::npos,
    "parser should describe an oversized warp coordinate pair"
);
error_message.clear();
expect(
    !parse_step_command(
        "loop-warp -1 = (X, Y)",
        &error_message
    ).has_value(),
    "parser should reject a negative loop count"
);
expect(
    error_message.find("nonnegative integer") != std::string::npos,
    "parser should describe an invalid loop count"
);
error_message.clear();
expect(
    !parse_step_command(
        "loop-warp = (X, Y)",
        &error_message
    ).has_value(),
    "parser should require a loop count"
);
expect(
    error_message.find("iteration count") != std::string::npos,
    "parser should describe a missing loop count"
);
error_message.clear();
expect(
    !parse_step_command(
        "loop-warp 2 = (X)",
        &error_message
    ).has_value(),
    "parser should validate the wrapped step's arguments"
);
expect(
    !error_message.empty(),
    "parser should return the wrapped step's validation error"
);
}
