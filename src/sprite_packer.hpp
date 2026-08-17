#ifndef SPRITE_PACKER_HPP
#define SPRITE_PACKER_HPP

#include "options.hpp"
#include "sprites.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

/** The metadata schema version written to each sheet's JSON file. Bump when the schema changes
 *  in a way that isn't backward-compatible, so engine-side importers can detect a mismatch. */
inline constexpr int k_metadata_schema_version{2};

/** A composed atlas image assembled in memory from one or more sprites. */
struct Atlas {
  std::vector<uint8_t> data;
  int width{}, height{};

  /** Encodes this atlas as a PNG and writes it to disk. */
  [[nodiscard]] std::expected<void, std::string> write(const std::filesystem::path &path) const;
};

/**
 * One packed sprite's placement and metadata, ready to export. `x/y/w/h` describe the trimmed
 * content's position within its sheet. `trim_x/trim_y` are the offsets from the *original*
 * (untrimmed) sprite's top-left corner to that trimmed content, and `source_width/source_height`
 * are the original sprite's dimensions — together these let an engine re-expand the sprite to its
 * authored bounding box. `pivot_x/pivot_y` are normalized (0..1) coordinates of the sprite's
 * anchor point. By default they're relative to the trimmed box (y=0 top, y=1 bottom) so pivot_y == 1
 * is the trimmed art's "feet" — a natural depth-sort key. With `--pivot full-canvas` they're
 * relative to the full source canvas with y measured from the BOTTOM, preserving the source
 * padding, and the atlas carries "pivot_basis": "full" so importers can tell the two apart.
 */
struct PackedSprite {
  std::string name;
  int sheet_index{};
  int x{}, y{}, w{}, h{};
  int trim_x{}, trim_y{};
  int source_width{}, source_height{};
  double pivot_x{}, pivot_y{};
};

/**
 * All data required to execute a sprite packing operation. Built from CLI options by
 * from_options(), which loads sprites (in parallel), optionally validates directional-animation
 * naming, trims and deduplicates them, and bin-packs the results into one or more atlas sheets
 * using MaxRectsPacker.
 */
struct PackData {
  std::filesystem::path sheets_dir;
  std::filesystem::path assets_dir;
  std::string output_name;

  bool full_canvas_pivot{false}; // when true, pivot_x/y are written as fractions of the full source
                                 // canvas (y from bottom) and the atlas carries "pivot_basis": "full"

  std::vector<Sprite> unique_images;    // deduplicated sprite pixel data, one entry per unique visual
  std::vector<TrimRect> unique_trims;   // trim rect for unique_images[i], in that sprite's own coordinates
  std::vector<PackedSprite> sprites;    // one entry per ORIGINAL input file (duplicates share placement)
  std::vector<int> sprite_unique_index; // sprites[i] was blitted from unique_images[sprite_unique_index[i]]

  std::vector<std::pair<int, int>> sheet_sizes; // final (width, height) for each sheet
  int num_sheets{};

  /** Validates options, loads all sprites, and computes the full pack data. */
  [[nodiscard]] static std::expected<PackData, std::string> from_options(const Options &options);

  /** Blits sprites into atlas sheets and writes a PNG file for each sheet. */
  [[nodiscard]] std::expected<void, std::string> pack() const;

  /** Writes a JSON metadata file for each atlas sheet. */
  [[nodiscard]] std::expected<void, std::string> write_metadata() const;
};

#endif // SPRITE_PACKER_HPP
