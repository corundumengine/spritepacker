# Spritepacker

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-23-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B23)

A CLI tool for packing sprite frames into texture atlases. **Pre-alpha — under active development, expect breakage.**

## Build

### Release

```bash
cmake --preset release
cmake --build build-release
cmake --install build-release --prefix dist
```

The executable will be at `dist/bin/spritepacker`.

### Debug

```bash
cmake --preset debug
cmake --build build
```

Requires a C++23 compiler and CMake 3.28+. Dependencies (nlohmann/json,
lodepng, doctest) are fetched automatically via FetchContent.

## Usage

```bash
spritepacker \
  --input ./assets/tiles \
  --output ./assets/atlases \
  --name terrain \
  --size 64x64
```

### Options

| Flag                 | Description                                                                           |
| -------------------- | ------------------------------------------------------------------------------------- |
| `--input, -i <dir>`  | Source directory containing PNG files (required)                                      |
| `--output, -o <dir>` | Output directory for atlas PNG and JSON (required)                                    |
| `--name, -n <name>`  | Base name for output files, e.g. `terrain` → `terrain.png`, `terrain.json` (required) |
| `--files, -f <list>` | Comma-separated filenames or glob patterns (default: `*.png`)                         |
| `--size, -s WxH`     | Frame size, e.g. `64x64`. Omit to auto-detect from input files                        |
| `--max-size, -m WxH` | Maximum atlas dimensions (default: `2048x2048`)                                       |
| `--help, -h`         | Show usage information                                                                |
| `--version, -v`      | Print version number                                                                  |

### Examples

Pack all PNGs from a tile directory with auto-detected frame sizes:

```bash
spritepacker --input tiles/terrain --output dist/atlases --name terrain
```

Pack a specific set of object sprites with an explicit frame size:

```bash
spritepacker -i sprites/objects -f chest_open.png,chest_closed.png -o dist/atlases -n chests -s 32x32
```

## Output

For each atlas, Spritepacker generates:

- **`{name}.png`** — The packed texture atlas image.
- **`{name}.json`** — Metadata describing the sprite sheet layout.

## Dependencies

| Library                                           | Purpose             |
| ------------------------------------------------- | ------------------- |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing/output |
| [lodepng](https://github.com/lvandeve/lodepng)    | PNG encoding        |
| [doctest](https://github.com/doctest/doctest)     | Unit testing        |

## Testing

The release preset enables tests by default. Run:

```bash
ctest --test-dir build-release --output-on-failure
```

Or with a debug build:

```bash
ctest --test-dir build --output-on-failure
```

## License

Apache-2.0 — see [LICENSE](LICENSE) for details.
