# Processor reference

Processors run sequentially from left to right. In a pattern file, put each
command in the ordered `processors` list. See the [usage guide](USAGE.md) for
the recommended YAML workflow, and see [`patterns/`](patterns) for runnable
examples.

## Processor commands

### Layout

- [`rotate <int>`](#rotate) rotates the image in quarter turns.
- [`mirror x`](#mirror) mirrors the image horizontally.
- [`mirror y`](#mirror) mirrors the image vertically.

### Filters

- [`blur <radius>`](#blur) applies a box blur.
- [`lighting <preset> [strength]`](#lighting) applies a lighting preset.
- [`lighting <angle> <#RRGGBB> <threshold|auto> [strength [softness [atmosphere]]]`](#lighting)
  creates a custom directional gel light.

### Toolbox

- [`black-and-white <brightness-multiplier>`](#black-and-white) converts the
  image to perceptual grayscale.
- [`contrast <factor>`](#contrast) changes contrast around the RGB midpoint.
- [`fisheye <x> <y> <amount> [radius]`](#fisheye) applies a radial lens.
- [`spin <x> <y> <angle> [radius]`](#spin) rotates pixels around a point.
- [`twist <x> <y> <force> [radius]`](#twist) twists pixels around a point.

### Drawing

- [`flow-lines <spacing> <steps> <step-size> <width> <#RRGGBB> [opacity] = (<VX>, <VY>)`](#flow-lines)
  draws streamlines through a formula-defined vector field.

### Formulas

- [`<channels> [offset <x> <y> [radius]] = <formula-or-tuple>`](#rgb-formula)
  replaces a subset of the RGB channels, optionally around a chosen point.
- [`s = <formula>`](#saturation-formula) changes HSL saturation.
- [`warp = (<source-x>, <source-y>)`](#warp-formula) remaps pixels with
  mathematical source coordinates.

### Localized

- [`local-rgb = (<red>, <green>, <blue>)`](#local-rgb) evaluates RGB formulas
  that can sample neighboring pixels.
- [`local-warp = (<source-x>, <source-y>)`](#local-warp) remaps pixels with
  formulas that can sample neighboring colors.

### Loops

- [`loop-rgb <iterations> = (<red>, <green>, <blue>)`](#loop-rgb) repeatedly
  applies an RGB formula.
- [`loop-warp <iterations> = (<source-x>, <source-y>)`](#loop-warp) repeatedly
  applies a warp formula.

[Back to top](#processor-reference)

## Formula expressions

Formulas support numeric constants, parentheses, unary signs, and `+`, `-`,
`*`, and `/`. Identifiers and function names are case-insensitive.

Color formulas can use `R`, `G`, and `B`, plus these image variables:

- `X`, `Y`: zero-based pixel coordinates.
- `W`, `H`: image width and height.
- `U`, `V`: coordinates normalized to `[-1, 1]`. A one-pixel dimension has a
  normalized coordinate of `0`.
- `D`: distance in pixels from the image center.
- `A`: angle in radians around the image center. Zero points right; positive
  angles turn clockwise in image space.
- `PI`, `E`: mathematical constants.

Available functions are:

- One argument: `sin`, `cos`, `tan`, `sqrt`, `abs`, `floor`, `ceil`, `round`,
  `exp`, and `log`.
- Two arguments: `atan2`, `pow`, `mod`, `min`, and `max`.
- Three arguments: `clamp(value, lower, upper)`. Reversed bounds are accepted.

Trigonometric functions use radians. Color and saturation results are rounded
and clamped to `[0, 255]`. Undefined results, such as the square root of a
negative number, become `0`.

Formula expressions may also use macros declared by `--macro` or a pattern
file's top-level `macros` section. Macro names must explicitly start with
`macro_`; their values are formula expressions and can reference other macros.
All CLI and file macros are collected before processing starts. Different
definitions for the same name are rejected, and unknown or cyclic macros fail
when a formula tries to resolve them.

Saturation formulas use the same coordinates, constants, and functions with
`S`, where saturation is represented in `[0, 255]`.

[Back to top](#processor-reference)

## Command details

### Layout

#### Rotate

```text
rotate <int>
```

Rotates clockwise by 90 degrees `int % 4` times. Negative values rotate in the
opposite direction.

[Back to top](#processor-reference)

#### Mirror

```text
mirror x
mirror y
```

`mirror x` mirrors the image horizontally. `mirror y` mirrors it vertically.

[Back to top](#processor-reference)

### Filters

#### Blur

```text
blur <radius>
```

Applies a box blur to RGB using a nonnegative integer pixel radius. Edge pixels
average only the available neighborhood; alpha is preserved.

[Back to top](#processor-reference)

#### Lighting

```text
lighting <preset> [strength]
lighting <angle> <#RRGGBB> <threshold|auto> [strength [softness [atmosphere]]]
```

The preset form accepts `golden-hour`, `moonlight`, `studio`, or `synthwave`.
Its optional strength is from `0` to `1`. Presets choose a palette, direction,
softness, atmospheric fill, and shadow balance. They use automatic luminance
analysis and adapt to dark illustrations and bright photographs.

```text
lighting golden-hour
lighting studio 0.8
lighting synthwave
```

For custom light, the angle describes where the source sits: `0` is right,
`90` is below, `180` is left, and `270` is above.

```text
lighting 315 #FF9A62 auto 0.85 12 0.08
```

Custom controls after the color are:

- `threshold`: luminance from `0` to `255`, or `auto`.
- `strength`: overall amount from `0` to `1`; default `0.78`.
- `softness`: penumbra as a percentage of the shorter dimension, from `0` to
  `50`; default `8`.
- `atmosphere`: color reaching regions below the threshold, from `0` to `1`;
  default `0.06`.

Multiple lighting commands compose from left to right.

[Back to top](#processor-reference)

### Toolbox

#### Black and white

```text
black-and-white <brightness-multiplier>
```

Converts RGB to perceptual grayscale and scales luminance by a nonnegative
finite multiplier. `1` preserves calculated brightness, values above `1`
brighten, values between `0` and `1` darken, and `0` produces black. RGB is
clamped to `[0, 255]`; alpha is preserved.

[Back to top](#processor-reference)

#### Contrast

```text
contrast <factor>
```

Changes RGB contrast around the midpoint. `1` leaves the image unchanged;
larger finite factors move dark values toward black and light values toward
white. Alpha is preserved.

[Back to top](#processor-reference)

#### Fisheye

```text
fisheye <x> <y> <amount> [radius]
```

`x` and `y` are percentages from `0` to `100`; `50 50` selects the image
center. An amount of `0` leaves the image unchanged. Positive amounts enlarge
the area around the point; local magnification is approximately `1 + amount`.
Negative values shrink the area and must be greater than `-1`.

The optional radius is a percentage of the shorter image dimension and
defaults to `100`. For example:

```text
fisheye 50 50 1 25
```

This limits the effect to a circle whose radius is 25% of that dimension. The
distortion tapers to zero at the boundary, and outside pixels are unchanged.

[Back to top](#processor-reference)

#### Spin

```text
spin <x> <y> <angle> [radius]
```

Rotates source coordinates by a fixed angle in degrees around percentage
coordinates. Negative angles reverse direction. The optional radius is a
percentage of the shorter image dimension; pixels outside it are unchanged.

[Back to top](#processor-reference)

#### Twist

```text
twist <x> <y> <force> [radius]
```

Twists around percentage coordinates. Rotation increases with distance from
the center at `force` radians per 100 pixels. Negative values reverse
direction. The optional radius is a percentage of the shorter image dimension;
the twist fades to zero at its boundary.

[Back to top](#processor-reference)

### Drawing

#### Flow lines

```text
flow-lines <spacing> <steps> <step-size> <width> <#RRGGBB> [opacity] = (<VX>, <VY>)
```

The two expressions after `=` define the horizontal and vertical components of
a vector field. The tracer normalizes each vector, so `step-size` controls
travel distance while the expressions control direction:

```text
flow-lines 24 800 1.25 1.2 #173F70 = (1, 0)
flow-lines 24 800 1.25 1.2 #173F70 0.85 = (-V, U)
```

Arguments are:

- `spacing`: distance between initial grid seeds and minimum occupancy-map
  separation.
- `steps`: maximum RK4 steps in each direction from a seed.
- `step-size`: pixel distance advanced by each integration step.
- `width`: antialiased stroke width in pixels.
- `#RRGGBB`: constant stroke color.
- `opacity`: optional value from `0` to `1`, defaulting to `1`.
- `(VX, VY)`: vector direction equations using the standard formula variables
  and functions. At fractional positions, RGB is bilinearly sampled.

The renderer composites over the current image without changing alpha. Invalid
or zero-length vectors terminate the affected path.

[Back to top](#processor-reference)

### Formulas

#### RGB formula

```text
<channels> = <formula-or-tuple>
<channels> offset <x> <y> [radius] = <formula-or-tuple>
```

Replaces any nonempty, non-repeating subset of `r`, `g`, and `b`. Target order
determines which expression writes each channel, and each expression reads the
original pixel.

A multi-channel tuple evaluates every expression against the original pixel.
This rotates the channels:

```text
rgb = (G, B, R)
```

The target can encode a swap while leaving other channels untouched:

```text
br = (R, B)
```

Separate assignments behave differently because later processors see the
changes made by earlier processors.

The optional `offset` moves the origin used by `A` and `D`. Its `x` and `y`
values are percentages from `0` to `100`: `0 0` is the top-left pixel,
`50 50` is the image center, and `100 100` is the bottom-right pixel. An
optional positive radius, also a percentage of the shorter image dimension,
limits the formula to that circle. Pixels on or outside the boundary are
unchanged, matching `spin` localization. For example, this moves radial bands
to 25% from the left and 70% from the top:

```text
rgb offset 25 70 = (
  R + 127 * sin(A * 180 / 2),
  G + 127 * sin(A * 180 / 3),
  B + 127 * sin(A * 180 / 2.5)
)
```

This brightens only a circle around the selected point:

```text
rgb offset 53 60 30 = (R * 2, G * 2, B * 2)
```

[Back to top](#processor-reference)

#### Saturation formula

```text
s = <formula>
```

Changes HSL saturation while preserving hue and lightness. The expression uses
the variables, constants, and functions described in
[Formula expressions](#formula-expressions), with `S` representing the current
saturation in `[0, 255]`.

[Back to top](#processor-reference)

#### Warp formula

```text
warp = (<source-x>, <source-y>)
```

A warp formula is evaluated for every output pixel. Its two expressions select
the source coordinate to sample from the unmodified input. Sampling is
bilinear, coordinates outside the image clamp to its edges, and output
dimensions do not change.

Horizontal waves:

```text
warp = (X + 20 * sin(Y / 15), Y)
```

Radial ripples:

```text
warp = (X + 12 * cos(A) * sin(D / 6), Y + 12 * sin(A) * sin(D / 6))
```

[Back to top](#processor-reference)

### Localized

#### Local RGB

```text
local-rgb = (<red>, <green>, <blue>)
```

Changes all three channels using formulas that can sample the unmodified input
around the current pixel with:

- `red(dx, dy)`
- `green(dx, dy)`
- `blue(dx, dy)`

Offsets may be fractional or formula-defined. Sampling is bilinear, and
coordinates beyond an edge clamp to that edge. All output pixels read the same
unmodified source, so results do not depend on traversal order. Alpha is
preserved from the current source pixel.

For example, a diagonal color emboss compares pixels on opposite sides:

```text
local-rgb = (128 + 2 * (red(2, 2) - red(-2, -2)), 128 + 2 * (green(2, 2) - green(-2, -2)), 128 + 2 * (blue(2, 2) - blue(-2, -2)))
```

[Back to top](#processor-reference)

#### Local warp

```text
local-warp = (<source-x>, <source-y>)
```

Remaps pixels using formulas that can sample neighboring colors from the
unmodified input. It adds the `red`, `green`, and `blue` neighbor-sampling
functions:

```text
local-warp = (X + (red(1, 0) - red(-1, 0)) / 32, Y)
```

Both the color lookups and final pixel lookup use the same unmodified input.

[Back to top](#processor-reference)

### Loops

#### Loop RGB

```text
loop-rgb <iterations> = (<red>, <green>, <blue>)
```

Treats an RGB formula as a discrete system in color space. Each tuple is
evaluated simultaneously, then its complete output becomes the next
iteration's input:

```text
loop-rgb 7 = (mod(R * 1.17 + G * 0.31 + 17, 256), mod(G * 1.13 + B * 0.29 + 31, 256), mod(B * 1.11 + R * 0.27 + 47, 256))
```

An iteration count of `0` leaves the image unchanged. Coordinates retain the
same meaning in every iteration.

[Back to top](#processor-reference)

#### Loop warp

```text
loop-warp <iterations> = (<source-x>, <source-y>)
```

Builds a transformation from smaller repeated steps:

```text
loop-warp 12 = ((W - 1) / 2 + D * cos(A + 0.01), (H - 1) / 2 + D * sin(A + 0.01))
```

Each iteration samples the preceding iteration. An iteration count of `0`
leaves the image unchanged.

[Back to top](#processor-reference)

## More examples

The [`patterns/`](patterns) directory is the best place to start. Its YAML
files are complete, runnable image recipes rather than isolated command
snippets.

[Back to top](#processor-reference)
