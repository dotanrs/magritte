# Step selection and image flow

This document explains how Magritte selects steps for command strings and
passes `FileData` from one step to the next.

The main implementation points are
[`input_parsing/step_command_parser.cpp`](input_parsing/step_command_parser.cpp),
[`step_runner.cpp`](step_runner.cpp), and
[`ImageStep`](../include/magritte/steps/image_step.h).

## Preparing a pipeline

The CLI accepts step commands directly with `-s` and indirectly through
YAML pattern files. Before image processing starts,
[`process_pipeline()`](pipeline.cpp) resolves pattern references, preserves the
declared command order, combines macros, and chooses the initial image:

- With `--source`, the initial `FileData` is decoded from the input JPEG.
- Without `--source`, a pattern-provided canvas becomes a black, opaque
  `FileData` buffer.

Both paths eventually produce an ordered `std::vector<StepSpec>`. A
`StepSpec` contains the original command and an optional descriptive name.

```mermaid
flowchart LR
    cli["CLI -s commands"]
    yaml["YAML pattern steps"]
    resolve["Resolve pipeline order,<br/>pattern references, and macros"]
    source["Decode source JPEG"]
    canvas["Create black RGBA canvas"]
    specs["Ordered StepSpec list"]
    process["process_file"]

    cli --> resolve
    yaml --> resolve
    resolve --> specs
    source --> process
    canvas --> process
    specs --> process
```

## How a step is selected

`parse_steps()` sends each command string to
`parse_step_command()`. That function trims the command and visits the
step registry in order.

For each registered `ImageStep`:

1. `parse_arguments(command)` checks whether the syntax belongs to that
   step.
2. A non-match returns `std::nullopt`, so registry scanning continues.
3. A match returns validated argument strings.
4. A matching but malformed command throws `std::invalid_argument`. The error
   is captured for reporting, and that command is not applied.

```mermaid
flowchart TD
    command["One command string"]
    first["Try registered step"]
    match{"Recognized?"}
    malformed{"Arguments valid?"}
    next["Try next step"]
    selected["Create StepCommand<br/>step reference + arguments + source"]
    skipped["Record error and skip command"]
    unknown["Record unknown step"]

    command --> first --> match
    match -- no --> next --> match
    match -- yes --> malformed
    malformed -- yes --> selected
    malformed -- no --> skipped
    match -- no steps left --> unknown
```

Registry order matters when syntaxes overlap. The registry in
[`input_parsing/step_command_parser.cpp`](input_parsing/step_command_parser.cpp) currently contains:

1. Rotate, mirror, blur.
2. Black-and-white, contrast, fisheye, twist, spin.
3. Flow lines and lighting.
4. RGB, local RGB, local warp, and saturation formulas.
5. Loop RGB, loop warp, and warp formula.

Keyword steps usually recognize commands such as `blur 3`.
`AssignmentStep` provides recognition for fixed assignments such as
`warp = (...)`. The RGB formula step has custom recognition because its
left-hand side can be any ordered, non-repeating subset of `r`, `g`, and `b`.

The result of selection is a `StepCommand`. It retains a reference to the
stateless step singleton, its validated argument vector, and the original
source command.

Invalid commands are reported and skipped; they do not prevent other valid
commands in the same pipeline from running.

## Sequential data flow

`run_steps()` owns the current `FileData` and executes the selected commands
strictly from left to right:

```cpp
data = command.step.get().apply(
    std::move(data),
    command.arguments,
    macros
);
```

Each result becomes the next step’s input:

```mermaid
flowchart LR
    input["Initial FileData"]
    p1["Step 1<br/>apply"]
    d1["FileData result 1"]
    p2["Step 2<br/>apply"]
    d2["FileData result 2"]
    p3["Step 3<br/>apply"]
    output["Final FileData"]

    input --> p1 --> d1 --> p2 --> d2 --> p3 --> output
```

This is why command order is observable. For example, `mirror x` followed by
`blur 3` blurs the mirrored pixels, while reversing the commands mirrors the
already blurred result.

The global macro map is passed to every step. Steps that do not use
formulas ignore it; formula-aware steps resolve their expression macros
when `apply()` begins.

Loop steps preserve the same flow at a smaller scale. They wrap another
step and repeatedly move each iteration’s result into the next
iteration.

## Why `apply()` takes `FileData` by value

The interface is:

```cpp
virtual FileData apply(
    FileData data,
    const std::vector<std::string> &arguments,
    const MacroMap *macros
) const;
```

Although the parameter is by value, `run_steps()` passes it with
`std::move`. The step therefore receives ownership of the current image
buffer without requiring a deep copy. It can choose the algorithm that fits
its transformation:

- Mutate the owned pixel vector and return it.
- Keep the owned buffer as an immutable source, allocate a destination buffer,
  and return the destination.
- Make a source snapshot while updating the owned buffer.

That choice is internal to the step. The pipeline always sees the same
contract: one `FileData` enters and one valid `FileData` leaves.

## In-place and out-of-place steps

“In-place” here means that the step mutates the pixel vector it received
by value. The caller has already moved ownership to it, so this does not mutate
an image still owned elsewhere.

| Strategy | Steps | Reason |
| --- | --- | --- |
| In-place | Mirror, black-and-white, contrast, RGB formula, saturation formula, flow lines | Each output can be written safely without needing an untouched full-image source during those writes. |
| New destination buffer | Rotate, blur, fisheye, twist, spin, local RGB, warp, local warp | Output positions depend on a different layout or on source pixels that must remain unchanged until all output pixels are computed. |
| In-place with a source snapshot | Lighting | Lighting writes into the owned image but retains a copy for stable luminance and occlusion calculations. |
| Delegated | Loop RGB and loop warp | The wrapped step decides; each returned result feeds the next iteration. |

Some details are worth noting:

- Rotate allocates a buffer with swapped dimensions for every quarter turn.
- Blur starts with a copy so neighborhood reads always observe the unblurred
  image.
- Fisheye, twist, spin, and warp compute a source coordinate for each
  destination pixel and write the sampled color into a separate result.
- Local formulas sample the immutable input, so they cannot safely overwrite
  that same buffer while traversing it.
- Flow lines compute their coverage before compositing it into the owned
  buffer, allowing the final blend to happen in-place.

An implementation may change strategy later without changing the
`ImageStep` interface.

## Debug hints and invariants

When debug mode is enabled, `run_steps()` calls `add_debug_hints()` after
the main transformation. Most steps inherit the default implementation,
which returns the image unchanged. Steps with a visualization overlay may
modify the already transformed result.

After every step and optional debug overlay,
`validate_file_data()` verifies:

- Width and height are nonzero.
- `width * height` does not overflow.
- The pixel vector contains exactly `width * height` entries.

This check catches a step that returns inconsistent dimensions or storage
before its result reaches the next command.

## End of the pipeline

After the last step:

- A source-based run sends the final `FileData` to `save_file()`.
- Valid and invalid step summaries are printed.

Output overwrite checks happen before decoding or processing. The file is only
encoded after all selected steps finish successfully.
