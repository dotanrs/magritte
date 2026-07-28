# Usage guide

`pixlie` can create an image from a blank canvas or transform an existing JPEG.
Although individual commands can be passed with `-p`, the recommended workflow
is to put the complete image recipe in a `.yml` formula and run it with `-f`.

See the [processor reference](PROCESSORS.md) for the available commands.

## Recommended formula workflow

### 1. Create a YAML formula

A formula contains an ordered list of processors. Include a `canvas` when the
formula creates a new image:

```yaml
canvas:
  file_name: "gradient.jpg"
  width: 480
  height: 320
processors:
  - name: warm gradient
    command: "rgb = (35 + 190 * X / W, 45 + 150 * Y / H, 170)"
  - name: add contrast
    command: "contrast 1.15"
```

The canvas starts black. Its `file_name` is resolved relative to the formula
file.

When transforming an existing image, the formula only needs its processors:

```yaml
processors:
  - name: horizontal mirror
    command: "mirror y"
  - name: gentle blur
    command: "blur 2"
```

Processors run from top to bottom, and each processor receives the result of
the previous one.

### 2. Run the formula

Create the image declared by a formula:

```sh
./build/pixlie -f path/to/formula.yml
```

Or apply a formula to a source JPEG:

```sh
./build/pixlie \
  --source photos/input.jpg \
  -f path/to/formula.yml \
  -o results/edited.jpg
```

When `--source` is present, canvases in formula files are ignored. If `-o` is
omitted, the default destination is the source name with `_copy` appended.
For example, `photos/input.jpg` becomes `photos/input_copy.jpg`.

Pass `-o` to override either the formula canvas's `file_name` or the default
source-image destination:

```sh
./build/pixlie -f formulas/canvas-gradient.yml -o results/gradient.jpg
```

### 3. Start from the examples

Runnable examples live in [`formulas/`](formulas). They demonstrate small
processor pipelines, source-image transformations, generated canvases, and
formula composition. Copy the closest example, change its processor commands,
and rerun it while developing an image.

## Formula file format

Formula files use the `.yml` or `.yaml` extension. The supported YAML subset is
intentionally small:

- A `canvas` requires `file_name`, a positive integer `width`, and a positive
  integer `height`.
- A processor item requires `name` and `command`.
- A formula-reference item contains only `formula`.
- Plain, single-quoted, and double-quoted scalars are accepted.
- Each processor command must stay on one line.

Formula files can include other formulas in the same ordered `processors`
list:

```yaml
canvas:
  file_name: "composition.jpg"
  width: 480
  height: 320
processors:
  - formula: "color-gradient.yml"
  - name: rotate clockwise
    command: "rotate 1"
  - name: increase contrast
    command: "contrast 1.15"
```

Relative sub-formula paths resolve beside the formula that references them.
A sub-formula's processors are inserted at the location of its reference, and
recursive reference cycles are rejected.

Without `--source`, the first processing argument must be a formula that
provides a canvas, directly or through a sub-formula. If more formulas follow,
their canvases are ignored; the first formula's canvas remains the pipeline
input.

## Combining formulas and processor arguments

Use `-p` for a quick, one-off processor:

```sh
./build/pixlie --source photo.jpg -o results/rotated.jpg \
  -p "rotate 1"
```

Both `-p` and `-f` can be repeated. They form one pipeline and run from left to
right in exactly the order supplied:

```sh
./build/pixlie --source photos/input.jpg \
  -p "rotate 1" \
  -f formulas/mirror-and-soften.yml \
  -p "contrast 1.2"
```

In this example, rotation runs first, the formula's processors are inserted
next, and contrast runs last.

## Debug mode

Add `--debug` to draw visual guides for processors that support them:

```sh
./build/pixlie \
  --source photo.jpg \
  -f formulas/puffy.yml \
  -o results/puffy-debug.jpg \
  --debug
```

Debug mode still applies the real transformation. Immediately after each
processor runs, `pixlie` draws that processor's guides onto the current image.
Those guides become part of the pipeline, so processors that run later can
also transform them. Processors without a debug visualization behave normally
and add nothing.

Currently supported guides are:

- `fisheye`: a yellow boundary and radius, with a magenta center crosshair.
- `twist`: the same boundary and center guides, plus a cyan curved line that
  shows direction and strength.
- `spin`: the same boundary and center guides, plus a straight cyan line that
  shows the fixed angle.

Because the guides are written into the output JPEG, use a separate output
path for debug runs when you also want a clean final image.

## Output and errors

If the output already exists, `pixlie` asks before replacing it. Use
`--overwrite` to replace it without prompting:

```sh
./build/pixlie -f formulas/canvas-gradient.yml --overwrite
```

The command writes timestamped progress logs to standard error. Invalid
processor commands are logged and skipped, so other valid processors can
still run. At the end, successful processors are printed in green and invalid
processors with their errors are printed in yellow.

Exit statuses are:

- `0`: success.
- `1`: file-processing error.
- `2`: invalid command-line arguments.

## Command-line options

```text
--source <path>  Source JPEG
-f <path>        Formula YAML; may be repeated
-p <command>     Processor command; may be repeated
-o <path>        Destination JPEG
--debug          Add supported visual processor guides to the output
--overwrite      Replace an existing output without prompting
-h, --help       Show command-line help
```
