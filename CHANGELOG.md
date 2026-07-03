# Changelog

All notable changes to this project are documented here.

## [0.2.0] — 2026-07-03

### Added

- **Crop mode (default)**: Each sprite is auto-cropped to its visible (non‑transparent) bounding box before packing. Cropped sprites are placed bottom‑center in uniform cells, and the JSON metadata includes `tile_width`, `tile_height`, `pivot_x` (0.5), and `pivot_y` (0.0).
- **`--margin` flag** (`--margin N`): Adds `N` transparent bleed pixels around each side of the cropped bounding box (default: `2`, crop mode only).
- **`--character` flag**: Disables cropping and preserves the original top‑left placement (for animated character sheets). Outputs `frame_width`/`frame_height`, `offset_x`/`y`, and `spacing_x`/`y` in JSON.
- **`--assets` flag** (`--assets <dir>`): Separate output directory for PNG sprite sheets. When omitted, PNGs are written alongside the JSON metadata (same as `--sheets`).

### Changed

- **Version bumped** from 0.1.0 → 0.2.0.
- **`--output` → `--sheets`**: Renamed the output directory flag to `--sheets` to clarify it holds JSON sheet metadata.
- **JSON output** no longer includes the `id`, `offset_x`, `offset_y`, `spacing_x`, or `spacing_y` fields in default (crop) mode. Character mode still outputs `offset_x`/`y` and `spacing_x`/`y`.
- **CLI help**: Updated to document the new `--character`, `--margin`, `--sheets`, and `--assets` flags.

### Fixed

- **`find_visible_bounds()`** correctly scans flat RGBA bytes (`Sprite::data`) instead of using a non‑existent `pixels` struct member.
- **Crop dimensions** are computed before `compute_layout()` so frame size reflects the cropped bounds rather than raw sprite sizes.

[0.2.0]: https://github.com/anomalyco/spritepacker/releases/tag/v0.2.0
