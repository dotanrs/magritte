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
- `fisheye <x> <y> <amount>` applies a radial lens centered on the given pixel coordinates. Positive amounts magnify;
  amounts between `-1` and `0` shrink.
- `r = <formula>` changes only the red channel.
- `g = <formula>` changes only the green channel.
- `b = <formula>` changes only the blue channel.
- `rgb = (<red>, <green>, <blue>)` changes all three channels simultaneously. Every expression reads the original pixel.
- `s = <formula>` changes HSL saturation while preserving hue and lightness.
- `warp = (<source-x>, <source-y>)` remaps pixels with mathematical source coordinates.
- `x <-> y` swaps channels `x` and `y`, where each channel is `r`, `g`, or
  `b`.

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

An RGB tuple is useful when output channels depend on one another. For example, this rotates the channels using the
original `R`, `G`, and `B` values:

```sh
-p "rgb = (G, B, R)"
```

Three separate assignments would behave differently because processors run sequentially and later formulas see changes
made by earlier processors.

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

### Fisheye shortcut

`fisheye` is a convenient radial warp around an arbitrary point:

```sh
-p "fisheye 640 360 1"
```

`x` and `y` are zero-based pixel coordinates and may be fractional. An amount of `0` leaves the image unchanged.
Positive amounts enlarge the area around the point; the local magnification is approximately `1 + amount`, so `1`
starts at about `2x`. Negative amounts shrink it, with `-0.5` starting at about `0.5x`. The amount must be greater than
`-1`, and the effect tapers with distance from the selected point.

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
