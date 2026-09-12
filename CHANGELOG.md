# Changelog

All notable changes to this project are documented here.

## [0.4.1] — 2026-09-11

### Added

- **`format_code` target and `format` build preset**: run `clang-format` over every first-party source with `cmake --build --preset format`.
- **`scripts/run_tidy.sh`**: run `clang-tidy` over an explicit file list against the project compile database.
- **`relwithdebinfo` and `debug-sanitized` presets** (ASan + UBSan), each with matching build and test presets.

### Changed

- **Redesigned CLI output**: `--help` is now grouped (Input/Output/Layout/Behavior) and trimmed to a scannable length, and a run prints numbered `[n/3]` steps followed by a summary panel instead of per-file listings and flat status lines.
- **Build now prefers LLVM/Clang**: the compiler is pinned to a discovered LLVM install (`LLVM_PREFIX`, Homebrew, `llvm-config`, the Windows LLVM installer, or `clang++` on `PATH`), falling back to the system compiler when none is found. First-party targets are now built with `-Werror`.
- **Build presets renamed**: `build-debug`/`build-release` are now `build`/`release`.

## [0.4.0] — 2026-08-16

### Added

- **`full-canvas` pivot mode**: `--pivot full-canvas` anchors the sprite on its FULL source canvas (y measured from the bottom), preserving authored padding where tilemap alignment often lives — instead of always anchoring the trimmed art. Takes an optional value suffix so you don't have to hand-edit every sprite afterwards: `full-canvas`, `full-canvas:0.18` (sets y), or `full-canvas:0.5,0.18` (sets x and y).
- **`pivot_basis` metadata field**: When built with `--pivot full-canvas`, each sheet's JSON carries `"pivot_basis": "full"` so importers can tell full-canvas pivots (y from the bottom) apart from the default trimmed-box pivots (y from the top).

### Removed

- **`--pivot-manifest` flag**: Removed per-sprite pivot overrides from a JSON file. `--pivot full-canvas` with its optional `:Y` / `:X,Y` suffix covers the same use case (anchoring on the source canvas rather than the trimmed art) without a separate file.

### Changed

- **Pivot parsing is now validated and centralized**: `--pivot` values are fully parsed by `resolve_pivot`, which reports errors for malformed `full-canvas` suffixes (e.g. `full-canvas:abc`) instead of silently defaulting to a wrong pivot. Pivot coordinates and their basis are carried as a single `Pivot` type shared by option validation and packing.

## [0.3.0] — 2026-07-08

### Added

- **MaxRects bin-packing**: Replaced the fixed-grid layout with a MaxRects packer, so sprites of varying sizes are packed tightly by trimmed content rather than uniform cells. Sprites are packed largest-first for a tighter layout.
- **Automatic trimming**: Every sprite is trimmed to its non-transparent bounding box before packing; the JSON metadata records each sprite's `trim_x`/`trim_y`/`source_width`/`source_height` so a renderer can restore its original position.
- **`--padding` flag** (`-p <n>`): Pixel gap between packed sprites (default: `1`).
- **`--pivot` flag**: Sprite anchor point preset — `bottom-center` (default), `center`, `top-center`, or `top-left`.
- **`--pivot-manifest` flag**: JSON file of per-sprite pivot overrides, keyed by sprite name.
- **`--pot` flag**: Rounds each sheet's final width/height up to the next power of two.
- **`--validate-animations` flag**: Requires `<unit>_<state>_<facing>_<frame>.png` sets to have matching frame counts across all facings of the same animation; fails the build if not.
- **Sprite deduplication**: Identical frames (by trimmed pixel content) reused across states are packed once; every referencing name in the exported metadata points at the single packed instance.
- **Parallel sprite loading**: PNG decoding is now done concurrently across worker threads.
- **`schema_version` field** in JSON metadata (currently `2`) to let consumers detect format changes.

### Changed

- **JSON metadata schema replaced**: `columns`/`rows`/`frame_width`/`frame_height`/`offset_*`/`spacing_*` are gone. Each sheet now has `width`/`height` and a `sprites` array of `{name, x, y, w, h, trim_x, trim_y, source_width, source_height, pivot_x, pivot_y}`.
- **`--size` flag removed**: Frame size is no longer a concept — sprites keep their own trimmed dimensions instead of being placed into a uniform grid cell.
- **Oversize sprites now fail the build** instead of only warning: a sprite (after trimming, plus padding) that exceeds `--max-size` is an error, not a silently-cropped warning.

## [0.2.1] — 2026-07-04

### Added

- **`--size` auto-expands**: When a `--size` hint is provided but a sprite exceeds it, the frame grows to fit rather than erroring. `--size` now serves as a minimum/direction frame size.

### Changed

- **Sprite packing is now the only mode**: Removed `--crop`, `--suggest-groups`, `--write-groups`, `--tolerance`, and `--margin`. The tool always preserves full sprite dimensions and writes `frame_width`/`frame_height` + `offset`/`spacing` metadata.
- **JSON metadata format** is now uniform: `frame_width`, `frame_height`, `offset_x`, `offset_y`, `spacing_x`, `spacing_y` for all sheets. No more `tile_width`/`tile_height` / `pivot` fields.

## [0.2.0] — 2026-07-03

### Added

- **Crop mode (default)**: Each sprite is auto-cropped to its visible (non‑transparent) bounding box before packing. Cropped sprites are placed bottom‑center in uniform cells, and the JSON metadata includes `tile_width`, `tile_height`, `pivot_x` (0.5), and `pivot_y` (0.0).
- **`--margin` flag** (`--margin N`): Adds `N` transparent bleed pixels around each side of the cropped bounding box (default: `2`, crop mode only).
- **`--character` flag**: Disables cropping and preserves the original top‑left placement (for animated character sheets). Outputs `frame_width`/`frame_height`, `offset_x`/`y`, and `spacing_x`/`y` in JSON.
- **`--assets` flag** (`--assets <dir>`): Separate output directory for PNG sprite sheets. When omitted, PNGs are written alongside the JSON metadata (same as `--sheets`).

### Changed

- **`--output` → `--sheets`**: Renamed the output directory flag to `--sheets` to clarify it holds JSON sheet metadata.
- **JSON output** no longer includes the `id`, `offset_x`, `offset_y`, `spacing_x`, or `spacing_y` fields in default (crop) mode. Character mode still outputs `offset_x`/`y` and `spacing_x`/`y`.
- **CLI help**: Updated to document the new `--character`, `--margin`, `--sheets`, and `--assets` flags.

### Fixed

- **`find_visible_bounds()`** correctly scans flat RGBA bytes (`Sprite::data`) instead of using a non‑existent `pixels` struct member.
- **Crop dimensions** are computed before `compute_layout()` so frame size reflects the cropped bounds rather than raw sprite sizes.
