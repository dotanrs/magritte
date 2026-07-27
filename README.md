# pixlie

`pixlie` is a C++ command-line JPEG processor. Processor commands are supplied with repeatable `-p` flags and run from
left to right.

JPEG decoding and encoding currently use the macOS ImageIO framework.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Use

Normalize and save an image as `photo_copy.jpg`:

```sh
./build/pixlie photo.jpg
```

If the output file already exists, `pixlie` asks before replacing it. Pass
`--overwrite` to replace it without prompting:

```sh
./build/pixlie photo.jpg --output results/edited.jpg --overwrite
```

Rotate clockwise by 90 degrees:

```sh
./build/pixlie photo.jpg --output results/rotated.jpg \
  -p "rotate 1"
```

Mirror across the y-axis and apply a three-pixel-radius blur:

```sh
./build/pixlie photo.jpg --output results/soft-mirror.jpg \
  -p "mirror y" \
  -p "blur 3"
```

Run multiple processors sequentially:

```sh
./build/pixlie photo.jpg --output results/edited.jpg \
  -p "rotate 1" \
  -p "rgb = (r * 2 - g, (r + b) / 2, 255 - b)"
```

Add `--debug` (or `-d`) to include visual guides in the output:

```sh
./build/pixlie photo.jpg --output results/fisheye-debug.jpg --debug \
  -p "fisheye 50 50 1 25"
```

Debug mode runs the same processor commands and then adds any hints supported
by those processors. Currently only `fisheye` provides hints: it draws the
circle and radius in yellow and marks its center with a magenta crosshair.

Generate a radial interference pattern from the image coordinates:

```sh
./build/pixlie photo.jpg --output results/interference.jpg \
  -p "r = 127 + 127 * sin(D / 8 + A * 6)" \
  -p "g = 127 + 127 * sin(D / 11 - A * 4)" \
  -p "b = 127 + 127 * cos(D / 6)"
```

The command writes timestamped progress logs to standard error and exits with status `0` on success, `1` for
file-processing errors, or `2` for invalid command-line arguments.

Invalid processor commands are logged and skipped without preventing other processor commands from running. At the end,
`pixlie` prints successful processor commands in green followed by invalid commands and their errors in yellow when any
errors occurred.

## Processors

- `rotate <int>` rotates clockwise by 90 degrees `int % 4` times. Negative values rotate in the opposite direction.
- `mirror x` mirrors the image horizontally.
- `mirror y` mirrors the image vertically.
- `blur <radius>` applies a box blur to RGB using a nonnegative integer pixel radius. Pixels near an edge average only
  the available neighborhood; alpha is preserved.
- `fisheye <x> <y> <amount> [radius]` applies a radial lens centered on the given percentage coordinates. Positive
  amounts magnify; amounts between `-1` and `0` shrink. Radius is an optional percentage of the image's shorter
  dimension and defaults to `100`.
- `lighting <preset> [strength]` applies one of the ready-to-use `golden-hour`, `moonlight`, `studio`, or `synthwave`
  looks. Optional strength is from `0` to `1`.
- `lighting <angle> <#RRGGBB> <threshold|auto> [strength [softness [atmosphere]]]` creates a custom directional gel
  light. It uses image luminance as scene structure, rolls the light off across the frame, softens occlusion, and adds
  restrained shadow contrast instead of painting solid ray-shaped color blocks.
- `<channels> = <formula-or-tuple>` changes any nonempty subset of `r`, `g`,
  and `b`. A one-channel target uses one formula, such as `r = G`. A
  multi-channel target uses the same number of tuple values, such as
  `rg = (G, R)` or `bgr = (R, G, B)`. Target order determines which expression
  writes each channel, every expression reads the original pixel, and channels
  cannot be repeated.
- `loop-rgb <iterations> = (<red>, <green>, <blue>)` repeatedly applies an RGB formula, feeding each completed result
  into the next iteration.
- `local-rgb = (<red>, <green>, <blue>)` changes all three channels using formulas that can sample neighboring pixels
  from the unmodified input image.
- `s = <formula>` changes HSL saturation while preserving hue and lightness.
- `warp = (<source-x>, <source-y>)` remaps pixels with mathematical source coordinates.
- `loop-warp <iterations> = (<source-x>, <source-y>)` repeatedly applies a warp, feeding each result into the next.

Formulas support numeric constants, parentheses, unary signs, and `+`, `-`, `*`, and `/`. Identifiers and function
names are case-insensitive.

Color formulas can use `R`, `G`, and `B`, as well as these image variables:

- `X`, `Y`: zero-based pixel coordinates.
- `W`, `H`: image width and height.
- `U`, `V`: coordinates normalized to `[-1, 1]`. A dimension containing one pixel has normalized coordinate `0`.
- `D`: distance in pixels from the image center.
- `A`: angle in radians around the image center. Zero points right and positive angles turn clockwise in image space.
- `PI`, `E`: mathematical constants.

Available functions are:

- One argument: `sin`, `cos`, `tan`, `sqrt`, `abs`, `floor`, `ceil`, `round`, `exp`, and `log`.
- Two arguments: `atan2`, `pow`, `mod`, `min`, and `max`.
- Three arguments: `clamp(value, lower, upper)`. Reversed bounds are accepted.

Trigonometric functions use radians. Color and saturation formula results are rounded and clamped to `[0, 255]`;
undefined results such as the square root of a negative number become `0`.

Saturation formulas support the same coordinates, constants, and functions with `S`, where saturation is represented in
the range `[0, 255]`.

An RGB tuple is useful when output channels depend on one another. For example,
this rotates the channels using the original `R`, `G`, and `B` values:

```sh
-p "rgb = (G, B, R)"
```

The target can also encode a swap while leaving other channels untouched:

```sh
-p "br = (R, B)"
```

Three separate assignments would behave differently because processors run
sequentially and later formulas see changes made by earlier processors.

### Iterated RGB formulas

`loop-rgb` treats an RGB formula as a discrete dynamical system in color space. Each tuple is evaluated simultaneously,
then its complete output becomes the input to the next iteration:

```sh
-p "loop-rgb 7 = (\
  mod(R * 1.17 + G * 0.31 + 17, 256), \
  mod(G * 1.13 + B * 0.29 + 31, 256), \
  mod(B * 1.11 + R * 0.27 + 47, 256)\
)"
```

An iteration count of `0` leaves the image unchanged. Coordinates retain the same meaning in every iteration, while
`R`, `G`, and `B` come from the preceding iteration.

### Local RGB formulas

A `local-rgb` formula can read the unmodified input image around each current pixel with three additional functions:

- `red(dx, dy)`
- `green(dx, dy)`
- `blue(dx, dy)`

The arguments are offsets relative to the current `X`, `Y` coordinate. They may be fractional or formula-defined.
Sampling is bilinear, coordinates beyond an edge clamp to that edge, and interpolated channel values remain
floating-point until the final RGB result is rounded and clamped. The ordinary `R`, `G`, and `B` variables still mean
the source pixel at the current coordinate.

This diagonal color emboss compares pixels on opposite sides of the current pixel:

```sh
-p "local-rgb = (\
  128 + 2 * (red(2, 2) - red(-2, -2)), \
  128 + 2 * (green(2, 2) - green(-2, -2)), \
  128 + 2 * (blue(2, 2) - blue(-2, -2))\
)"
```

All output pixels read the same unmodified source, so results never depend on traversal order. Alpha is preserved from
the current source pixel.

### Warp formulas

A warp formula is evaluated once for every output pixel. Its two expressions select the source coordinate to sample from
the unmodified input image. Sampling is bilinear, coordinates outside the image clamp to its edges, and the output keeps
the original dimensions. `R`, `G`, and `B` refer to the input pixel at the current `X`, `Y` position.

Horizontal waves:

```sh
-p "warp = (X + 20 * sin(Y / 15), Y)"
```

Radial ripples:

```sh
-p "warp = (X + 12 * cos(A) * sin(D / 6), \
            Y + 12 * sin(A) * sin(D / 6))"
```

A distance-dependent swirl:

```sh
-p "warp = ((W - 1) / 2 + D * cos(A + D / 200), \
  (H - 1) / 2 + D * sin(A + D / 200))"
```

To build up a transformation over several smaller steps, use `loop-warp`.
This applies a slight rotation twelve times:

```sh
-p "loop-warp 12 = ((W - 1) / 2 + D * cos(A + 0.01), \
  (H - 1) / 2 + D * sin(A + 0.01))"
```

Each iteration samples the image produced by the preceding iteration. An
iteration count of `0` leaves the image unchanged.


### Lighting

Start with a preset; it chooses a color palette, direction, softness, atmospheric fill, and shadow balance:

```sh
# Warm light from the upper right
-p "lighting golden-hour"

# Warm key plus a cool rim light
-p "lighting studio 0.8"

# Pink and cyan lights from opposite sides
-p "lighting synthwave"
```

Presets use `auto` luminance analysis, so they adapt to both dark illustrations and bright photographs. For a custom
light, angles describe where the source sits: `0` is right, `90` is below, `180` is left, and `270` is above.

```sh
-p "lighting 315 #FF9A62 auto 0.85 12 0.08"
```

The custom controls after the color are:

- `threshold`: a luminance from `0` to `255`, or `auto` to estimate it from the image.
- `strength`: overall amount from `0` to `1`; defaults to `0.78`.
- `softness`: penumbra size as a percentage of the shorter image dimension, from `0` to `50`; defaults to `8`.
- `atmosphere`: how much color reaches regions below the luminance threshold, from `0` to `1`; defaults to `0.06`.

Use a low atmosphere for clean relighting and a higher value for a visible color wash. Multiple lighting commands still
compose from left to right.



```sh
-p "rgb = (255 - R, 255 - G, 255 - B)" \
-p "lighting 315 #FFB45C 150 0.9"
```

### Fisheye shortcut

`fisheye` is a convenient radial warp around an arbitrary point:

```sh
-p "fisheye 50 50 1"
```

`x` and `y` are percentages from `0` to `100`, so `50 50` selects the image center. They may be fractional. An amount
of `0` leaves the image unchanged.
Positive amounts enlarge the area around the point; the local magnification is approximately `1 + amount`, so `1`
starts at about `2x`. Negative amounts shrink it, with `-0.5` starting at about `0.5x`. The amount must be greater than
`-1`.

The optional radius is a percentage of the image's shorter dimension and defaults to `100`. For example,
`fisheye 50 50 1 25` limits the effect to a circle whose radius is 25% of that dimension. The distortion tapers
smoothly to zero at the circle's boundary, and pixels outside it remain unchanged.

### Formula examples

Each snippet below is a processor argument that can be appended to a `pixlie` command.

Horizontal red waves:

```sh
-p "r = 127 + 127 * sin(X / 12)"
```

Concentric blue rings:

```sh
-p "b = 127 + 127 * cos(D / 6)"
```

Green spokes radiating from the image center:

```sh
-p "g = 127 + 127 * sin(A * 12)"
```

A coordinate checker pattern mixed with the original red channel:

```sh
-p "r = R * (0.5 + 0.5 * sin(X / 8) * sin(Y / 8))"
```

Color contours derived from the original pixel brightness:

```sh
-p "b = 255 * abs(sin((R + G + B) / 24))"
```

A nonlinear red-channel color hash:

```sh
-p "r = mod(R * R + G * B, 256)"
```

A radial brightness falloff:

```sh
-p "r = R * (1 - D / max(W, H))" \
-p "g = G * (1 - D / max(W, H))" \
-p "b = B * (1 - D / max(W, H))"
```

Curved interference bands:

```sh
-p "r = 127 + 127 * sin((X * X + Y * Y) / 500)"
```

Saturation alternating in diagonal waves:

```sh
-p "s = S * (0.5 + 0.5 * sin((X + Y) / 16))"
```

The three-channel radial interference pattern used for the CLI smoke test:

```sh
-p "rgb = (127 + 127 * sin(D / 8 + A * 6), \
           127 + 127 * sin(D / 11 - A * 4), \
           127 + 127 * cos(D / 6))"
```

## Tests

```sh
ctest --test-dir build --output-on-failure
```
