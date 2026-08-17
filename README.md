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
  --sheets ./assets/atlases \
  --name terrain
```

Spritepacker trims transparent padding from each sprite, deduplicates pixel-identical frames, and
bin-packs the result into one or more atlas sheets with a MaxRects packer — each sprite only takes
up as much space as its visible content needs, rather than a fixed grid cell. This matters most for
isometric sprite sets, where a "walk" animation frame and a tiny prop icon can differ wildly in
trimmed size.

### Options

| Flag                    | Description                                                                                                                                                                    |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `--input, -i <dir>`     | Source directory containing PNG files (required)                                                                                                                               |
| `--sheets <dir>`        | Output directory for JSON sheet metadata (required)                                                                                                                            |
| `--assets <dir>`        | Output directory for PNG sprite sheets (default: same as `--sheets`)                                                                                                           |
| `--name, -n <name>`     | Base name for output files, e.g. `terrain` → `terrain.png`, `terrain.json` (required)                                                                                          |
| `--files, -f <list>`    | Comma-separated filenames or glob patterns (default: `*.png`)                                                                                                                  |
| `--max-size, -m WxH`    | Maximum atlas size per sheet (default: `2048x2048`); sprites that don't fit start a new sheet                                                                                  |
| `--padding, -p <n>`     | Pixel gap between packed sprites, to avoid texture-filtering bleed (default: `1`)                                                                                              |
| `--pivot <preset>`      | Sprite anchor — see [Pivot / depth-sort](#pivot--depth-sort). Presets: `bottom-center` (default), `center`, `top-center`, `top-left`, or `full-canvas[:Y]` / `full-canvas:X,Y` |
| `--validate-animations` | Require animation frame sets to match across facings — see [Animation naming](#animation-naming)                                                                               |
| `--pot`                 | Round each sheet's final width/height up to the next power of two                                                                                                              |
| `--help, -h`            | Show usage information                                                                                                                                                         |
| `--version, -v`         | Print version number                                                                                                                                                           |

### Examples

Pack all PNGs from a tile directory:

```bash
spritepacker --input tiles/terrain --sheets dist/atlases --name terrain
```

Pack a specific set of object sprites, with 2px padding between them:

```bash
spritepacker -i sprites/objects -f chest_open.png,chest_closed.png --sheets dist/atlases -n chests -p 2
```

Pack a full character's animations, failing the build if any facing is missing frames:

```bash
spritepacker -i sprites/knight -f "knight_*.png" --sheets dist/atlases -n knight --validate-animations
```

## Pivot / depth-sort

Every packed sprite gets a `pivot_x`/`pivot_y` in its metadata — normalized (0..1) coordinates of
its anchor point within its _trimmed_ bounding box, computed after trimming so it stays correct
regardless of how much transparent padding was cut away. The default (`bottom-center`, i.e.
`pivot_x=0.5, pivot_y=1.0`) points at a standing sprite's feet, which is also the standard
depth-sort key for isometric rendering — sort draw calls by each sprite's world-space `pivot_y` (or
equivalently, by the y-coordinate of the tile it stands on). There's no separate sort-hint field;
it's intentionally derived from the pivot rather than duplicated.

The `--pivot` presets (`bottom-center`, `center`, `top-center`, `top-left`) anchor the _trimmed_
art, with `y` measured from the top (the raster convention). That's right for characters and
objects, where the trimmed art's own "feet" is the anchor.

### `full-canvas` mode

For sprites where the anchor should sit somewhere on the _full_ source canvas — not the trimmed
content — use `full-canvas`. This measures `y` from the **bottom** (the engine's convention) and
preserves the source padding, which is where tilemap alignment often lives. It takes an optional
value suffix so you don't have to hand-edit every sprite afterwards:

| Flag                           | Result pivot  | Notes                     |
| ------------------------------ | ------------- | ------------------------- |
| `--pivot full-canvas`          | `(0.5, 0.0)`  | full-canvas bottom-center |
| `--pivot full-canvas:0.18`     | `(0.5, 0.18)` | y only                    |
| `--pivot full-canvas:0.5,0.18` | `(0.5, 0.18)` | x and y                   |

When `full-canvas` is used, the atlas metadata carries `"pivot_basis": "full"` so an importer can
tell pivots measured against the full canvas (y from the bottom) apart from the default trimmed-box
pivots (y from the top).

## Animation naming

`--validate-animations` checks that input filenames follow the
`<unit>_<state>_<facing>_<frame>.png` convention (e.g. `knight_walk_south_2.png`) and that, for
every `<unit>_<state>` animation with more than one facing, all facings share the same set of frame
indices. If a facing is missing a frame another facing has, the tool fails with a non-zero exit
code and a message naming exactly which animation, facing, and frame is missing — instead of
silently packing an incomplete set and only noticing at runtime. Filenames that don't match the
convention are ignored by this check (e.g. non-directional sprites like `chest_open.png`).

## Output

For each atlas sheet, Spritepacker generates:

- **`{name}.png`** (or `{name}-N.png` for sheet N when multiple sheets are needed) — the packed texture atlas.
- **`{name}.json`** (or `{name}-N.json`) — metadata for that sheet:

```json
{
  "schema_version": 2,
  "path": "dist/atlases/knight.png",
  "width": 256,
  "height": 128,
  "sprites": [
    {
      "name": "knight_walk_south_0",
      "x": 10,
      "y": 20,
      "w": 24,
      "h": 40,
      "trim_x": 8,
      "trim_y": 16,
      "source_width": 40,
      "source_height": 64,
      "pivot_x": 0.5,
      "pivot_y": 1.0
    }
  ]
}
```

`x/y/w/h` is the trimmed sprite's position in the atlas. `trim_x/trim_y` is the offset from the
_original_ (untrimmed) sprite's top-left corner to that trimmed region, and `source_width` /
`source_height` are the original dimensions — together these let an engine re-expand a sprite to
its authored bounding box before applying its pivot. Duplicate frames (pixel-identical after
trimming) are packed once; every filename that referenced that content gets its own `sprites`
entry pointing at the same `x/y/w/h`.

`schema_version` is bumped whenever this JSON schema changes in a backward-incompatible way, so an
engine-side importer can detect a stale reader instead of silently misreading fields. When built with
`--pivot full-canvas`, the atlas also carries `"pivot_basis": "full"` (see
[Pivot / depth-sort](#pivot--depth-sort)).

Compressed texture output (ASTC/ETC/BCn) is out of scope for this tool — sheets are written as PNG
and are expected to be compressed by a later build step if the target platform needs it.

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
