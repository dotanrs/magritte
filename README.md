# pixlie

`pixlie` is a C++ command-line JPEG processor. Processor commands are supplied
with repeatable `-p` flags and run from left to right.

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

Run multiple processors sequentially:

```sh
./build/pixlie photo.jpg --output results/edited.jpg \
  -p "rotate 1" \
  -p "r = r * 2 - g" \
  -p "g = (r + b) / 2" \
  -p "b = 255 - b"
```

The command writes timestamped progress logs to standard error and exits with
status `0` on success, `1` for file-processing errors, or `2` for invalid
command-line arguments.

Invalid processor commands are logged and skipped without preventing other
processor commands from running.

## Processors

- `rotate <int>` rotates clockwise by 90 degrees `int % 4` times. Negative
  values rotate in the opposite direction.
- `r = <formula>` changes only the red channel.
- `g = <formula>` changes only the green channel.
- `b = <formula>` changes only the blue channel.

Formulas support `R`, `G`, and `B`, numeric constants, parentheses, unary
signs, and `+`, `-`, `*`, and `/`. Results are rounded and clamped to the range
`[0, 255]`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```
