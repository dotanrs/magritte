# Usage guide

`magritte` can create an image from a blank canvas or transform an existing JPEG.
Although individual commands can be passed with `-s`, the recommended workflow
is to put the complete image recipe in a `.yml` pattern and run it with `-p`.

See the [step reference](STEPS.md) for the available commands.

## Recommended pattern workflow

### 1. Create a YAML pattern

A pattern contains an ordered list of steps. Include a `canvas` when the
pattern creates a new image:

```yaml
canvas:
  file_name: "gradient.jpg"
  width: 480
  height: 320
steps:
  - name: warm gradient
    command: "rgb = (35 + 190 * X / W, 45 + 150 * Y / H, 170)"
  - name: add contrast
    command: "contrast 1.15"
```

The canvas starts black. Its `file_name` is resolved relative to the pattern
file.

When transforming an existing image, the pattern only needs its steps:

```yaml
steps:
  - name: horizontal mirror
    command: "mirror y"
  - name: gentle blur
    command: "blur 2"
```

Steps run from top to bottom, and each step receives the result of
the previous one.

### 2. Run the pattern

Create the image declared by a pattern:

```sh
./build/magritte -p path/to/pattern.yml
```

Or apply a pattern to a source JPEG:

```sh
./build/magritte \
  --source photos/input.jpg \
  -p path/to/pattern.yml \
  -o results/edited.jpg
```

When `--source` is present, canvases in pattern files are ignored. If `-o` is
omitted, the default destination is the source name with `_copy` appended.
For example, `photos/input.jpg` becomes `photos/input_copy.jpg`.

Pass `-o` to override either the pattern canvas's `file_name` or the default
source-image destination:

```sh
./build/magritte -p patterns/canvas-gradient.yml -o results/gradient.jpg
```

### 3. Start from the examples

Runnable examples live in [`patterns/`](patterns). They demonstrate small
step pipelines, source-image transformations, generated canvases, and
pattern composition. Copy the closest example, change its step commands,
and rerun it while developing an image.

## Pattern file format

Pattern files use the `.yml` or `.yaml` extension. The supported YAML subset is
intentionally small:

- A `canvas` requires `file_name`, a positive integer `width`, and a positive
  integer `height`.
- An optional top-level `macros` section contains indented
  `macro_<name>=<formula>` definitions.
- A step item requires `name` and `command`.
- A pattern-reference item contains only `pattern`.
- Plain, single-quoted, double-quoted, folded (`>`), and literal (`|`) scalars
  are accepted.
- Use a folded block to split a long step command across readable lines.
  Its ordinary line breaks become spaces:

  ```yaml
  - name: horizontal wave
    command: >
      warp =
      (
      X + 20 * sin(Y / 15),
      Y
      )
  ```

- A literal block preserves its line breaks. Both block styles require content
  to be indented farther than the field containing `>` or `|`.

Pattern files can include other patterns in the same ordered `steps`
list:

```yaml
canvas:
  file_name: "composition.jpg"
  width: 480
  height: 320
steps:
  - pattern: "color-gradient.yml"
  - name: rotate clockwise
    command: "rotate 1"
  - name: increase contrast
    command: "contrast 1.15"
```

Relative nested-pattern paths resolve beside the pattern that references them.
A nested pattern's steps are inserted at the location of its reference, and
recursive reference cycles are rejected.

## Expression macros

Macros give repeated formula expressions explicit names. Every macro name must
start with `macro_`, making collisions with built-in variables and constants
visible and intentional:

```yaml
macros:
  macro_lower=pow(clamp((V + 0.36) / 1.36, 0, 1), 2)
  macro_wave=macro_lower * sin(Y / 18 + X / 95)
steps:
  - name: liquid wave
    command: "warp = (X + 32 * macro_wave, Y)"
```

Macro values are formula expressions and may reference other macros. Names are
case-insensitive when formulas use them. Unknown macros and cyclic references
are errors.

Add macros from the command line with a repeatable `--macro` option. Quote the
definition when its expression contains spaces or shell-sensitive characters:

```sh
./build/magritte --source photo.jpg \
  --macro "macro_gain=1.25 + 0.25 * V" \
  -s "rgb = (R * macro_gain, G, B)" \
  -o result.jpg
```

CLI macros and macros from every referenced pattern are collected before the
first step runs. Repeating an identical definition is allowed. If the
same name has different expressions, the run fails instead of selecting one
definition implicitly.

Without `--source`, the first processing argument must be a pattern that
provides a canvas, directly or through a nested pattern. If more patterns follow,
their canvases are ignored; the first pattern's canvas remains the pipeline
input.

## Combining patterns and step arguments

Use `-s` for a quick, one-off step:

```sh
./build/magritte --source photo.jpg -o results/rotated.jpg \
  -s "rotate 1"
```

Both `-s` and `-p` can be repeated. They form one pipeline and run from left to
right in exactly the order supplied:

```sh
./build/magritte --source photos/input.jpg \
  -s "rotate 1" \
  -p patterns/mirror-and-soften.yml \
  -s "contrast 1.2"
```

In this example, rotation runs first, the pattern's steps are inserted
next, and contrast runs last.

## Debug mode

Add `--debug` to draw visual guides for steps that support them:

```sh
./build/magritte \
  --source photo.jpg \
  -p patterns/puffy.yml \
  -o results/puffy-debug.jpg \
  --debug
```

Debug mode still applies the real transformation. Immediately after each
step runs, `magritte` draws that step's guides onto the current image.
Those guides become part of the pipeline, so steps that run later can
also transform them. Steps without a debug visualization behave normally
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

If the output already exists, `magritte` asks before replacing it. Use
`--overwrite` to replace it without prompting:

```sh
./build/magritte -p patterns/canvas-gradient.yml --overwrite
```

The command writes timestamped progress logs to standard error. Invalid
step commands are logged and skipped, so other valid steps can
still run. At the end, successful steps are printed in green and invalid
steps with their errors are printed in yellow.

Exit statuses are:

- `0`: success.
- `1`: file-processing error.
- `2`: invalid command-line arguments.

## Command-line options

```text
--source <path>  Source JPEG
-p, --pattern <path>
                 Pattern YAML; may be repeated
-s, --step <command>
                 Step command; may be repeated
--macro <macro_<name>=<formula>>
                 Global expression macro; may be repeated
-o <path>        Destination JPEG
--debug          Add supported visual step guides to the output
--overwrite      Replace an existing output without prompting
-h, --help       Show command-line help
```
