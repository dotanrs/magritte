# pixlie

`pixlie` is a C++ command-line image processor. This first version accepts a
JPEG image and creates an exact copy.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Use

Create `photo_copy.jpg` next to the source image:

```sh
./build/pixlie photo.jpg
```

Choose the output path:

```sh
./build/pixlie photo.jpg --output results/edited.jpg
```

The command writes timestamped progress logs to standard error and exits with
status `0` on success, `1` for file-processing errors, or `2` for invalid
command-line arguments.
