# Magritte

`magritte` is a C++20 command-line JPEG processor. It applies processor
commands from left to right to transform a source image or generate an image
from a blank canvas.

JPEG decoding and encoding currently use the macOS ImageIO framework.

## Quick Usage

Download and unzip [magritte.zip](https://github.com/dotanrs/magritte/blob/main/magritte.zip).

It contains the executable `magritte` and the relevant `.md` files and examples.

Run it manually (`magritte --help`) or use your favorite AI to run it for you. See the recommended workflow below for working with
fast and easy iterations on your designs.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Recommended workflow: create a pattern file

The recommended way to create an image is to describe it in a `.yml` pattern
file. This keeps the complete recipe readable, repeatable, and easy to adjust.

Create a file such as `my-image.yml`:

```yaml
canvas:
  file_name: "my-image.jpg"
  width: 480
  height: 320
processors:
  - name: warm gradient
    command: "rgb = (35 + 190 * X / W, 45 + 150 * Y / H, 170)"
  - name: increase contrast
    command: "contrast 1.15"
```

Then run it:

```sh
./build/magritte -P my-image.yml
```

To transform an existing JPEG, create a pattern without the `canvas` section
and provide a source:

```sh
./build/magritte --source photo.jpg -P photo-edit.yml -o edited.jpg
```

See [`patterns/`](patterns) for runnable examples, including generated
canvases, patterns that transform source images, and patterns that compose
other patterns.

## Examples

The examples below transform the shared
[`original.jpeg`](patterns/examples/original.jpeg) source image. Select an
example to view its pattern.

| Original |                                      Slenderman                                       |                                       Hitchcock                                       |
| :---: |:-------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------:|
| ![Original source image](patterns/examples/original.jpeg) |  [![Slenderman example](patterns/examples/slenderman.jpg)](patterns/slenderman.yml)   |        [![Hitchcock](patterns/examples/hitchcock.jpg)](patterns/hitchcock.yml)        |
| Source image |                      [`slenderman.yml`](patterns/slenderman.yml)                      |                       [`hitchcock.yml`](patterns/hitchcock.yml)                       |
| **Mass gain** |                                    **Iridescence**                                    |                                    **Telekinesis**                                    |
| [![Mass gain example](patterns/examples/mass-gain.jpg)](patterns/mass-gain.yml) | [![Iridescence example](patterns/examples/iridescence.jpg)](patterns/iridescence.yml) | [![Telekinesis example](patterns/examples/telekinesis.jpg)](patterns/telekinesis.yml) |
| [`mass-gain.yml`](patterns/mass-gain.yml) |                     [`iridescence.yml`](patterns/iridescence.yml)                     |                     [`telekinesis.yml`](patterns/telekinesis.yml)                     |

## Documentation

- [Usage guide](USAGE.md) — pattern files, source images, output options,
  command ordering, and debug mode.
- [Processor reference](PROCESSORS.md) — every processor and the expression
  language used by formula processors.
- [Formula tree construction](src/processors/formulas/utils/FORMULA_PARSER.md)
  — how expression nodes, identifiers, functions, and macros form an AST.
- [Processor selection and image flow](src/PROCESSING.md) — how commands select
  processors and pass image ownership through the pipeline.
- [JPEG and in-memory image I/O](src/io/IO.md) — how JPEG data is decoded into
  `FileData` and encoded again.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## Lint

Install `clang-tidy`, then configure a dedicated lint-enabled build:

```sh
cmake -S . -B build-lint -DMAGRITTE_ENABLE_CLANG_TIDY=ON
cmake --build build-lint
```

The lint rules are defined in [`.clang-tidy`](.clang-tidy). Lint findings are
treated as errors in lint-enabled builds.
