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
  -p "r = r * 2 - g" \
  -p "g = (r + b) / 2" \
  -p "b = 255 - b"
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
- `r = <formula>` changes only the red channel.
- `g = <formula>` changes only the green channel.
- `b = <formula>` changes only the blue channel.
- `s = <formula>` changes HSL saturation while preserving hue and lightness.
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

Trigonometric functions use radians. Formula results are rounded and clamped to `[0, 255]`; undefined results such as
the square root of a negative number become `0`.

Saturation formulas support the same coordinates, constants, and functions with `S`, where saturation is represented in
the range `[0, 255]`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```
