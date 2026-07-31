# Repository guide

`magritte` is a C++20 command-line JPEG processor. Processor commands are parsed
from left to right and applied sequentially to a `FileData` image.

## Folder structure

- `main.cpp` is the CLI entry point.
- `include/magritte/` contains the headers shared by the application and tests.
  Processor declarations live in `include/magritte/processors/`, while
  `common/` contains the image data types, `io/` exposes file I/O, and `utils/`
  contains general helpers.
- `src/` contains the CLI workflow, pattern-file support, and the processor
  registry in `src/parser.cpp`.
- `src/io/` contains JPEG input/output formatting, path and input validation,
  and its implementation notes.
- `src/processors/` contains processor implementations, grouped by behavior:
  - `layout/` changes image orientation or dimensions, such as rotate and
    mirror.
  - `filters/` derives output from the existing image, such as blur and
    lighting.
  - `toolbox/` contains self-contained creative transforms and adjustments,
    such as contrast, fisheye, and twist.
  - `drawing/` draws generated geometry over the image, such as flow lines.
  - `formulas/` evaluates mathematical expressions per pixel or remaps pixels
    from formula-defined coordinates.
  - `localized/` is formula processing that may sample neighboring pixels from
    the immutable input image.
  - `loops/` repeatedly applies another processor, feeding each result into the
    next iteration.
  - `formulas/utils/` contains shared formula parsing, evaluation, and image
    sampling code.
- `tests/processors/` contains focused processor tests. Shared assertions and
  image helpers live in `tests/common/`; `tests/processors_test.cpp` is the test
  runner.
- `patterns/` contains runnable YAML patterns that demonstrate supported processor
  commands.
- `CMakeLists.txt` declares both the CLI and processor-test targets.

## Processor types

Every processor implements `ImageProcessor`, which separates four concerns:
recognizing a command, validating its arguments, applying it, and optionally
adding debug hints.

- Keyword processors use commands such as `blur 3`. They usually parse
  arguments with `processor_argument_parse::after_keyword`.
- Assignment processors use commands such as `warp = (X, Y)`. When the
  left-hand side is a fixed keyword, derive from `AssignmentProcessor` to reuse
  recognition and validation flow. The RGB formula processor is a special
  assignment processor because its left-hand side may be any non-repeating
  subset of `r`, `g`, and `b`.
- Formula processors evaluate expressions independently for each output pixel.
  Normal RGB and saturation formulas read the current pixel and image
  coordinates; warp formulas interpret their results as source coordinates.
- Localized processors add relative, bilinearly interpolated sampling of the
  unmodified input. Use this type when a pixel's result depends on nearby source
  pixels rather than only its own channels and coordinates.
- Loop processors are wrappers around an existing processor. `LoopProcessor`
  handles iteration and result feedback, while `LoopAssignmentProcessor`
  supplies the `loop-<name> <count> = ...` syntax. Prefer wrapping an existing
  processor over duplicating its transformation.
- Drawing processors composite generated marks onto the input. They still obey
  the same parsing, validation, alpha-preservation, and image-invariant
  expectations as other processors.

The source folders describe behavior, not separate interfaces: all of these
types ultimately participate in the same `ImageProcessor` registry and
left-to-right pipeline.

## Adding a processor

For every new processor:

1. Add its declaration under `include/magritte/processors/` and place its
   implementation in the matching `src/processors/` behavior folder.
2. Register it in `src/parser.cpp`. Registry order matters when command
   syntaxes can overlap.
3. Add the new files to the CLI and test targets in `CMakeLists.txt`.
4. Add a focused test in `tests/processors/`, then declare and invoke that test
   from `tests/processors_test.cpp`. Cover command recognition and invalid
   arguments as well as the transformation when applicable.
5. Add a runnable example to an appropriate YAML pattern in `patterns/` (or create a
   focused pattern there) and document the command in `README.md`.

A new processor is not complete without both its test and its `patterns/` example.

Build and run the suite with:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
