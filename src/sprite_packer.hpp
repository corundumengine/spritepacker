#ifndef SPRITE_PACKER_HPP
#define SPRITE_PACKER_HPP

#include "options.hpp"
#include "sprites.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

/** A composed atlas image assembled in memory from one or more sprites. */
struct Atlas {
  std::vector<uint8_t> data;
  int width{}, height{};

  /** Encodes this atlas as a PNG and writes it to disk. */
  [[nodiscard]] std::expected<void, std::string> write(const std::filesystem::path &path) const;
};

/** Computes the grid layout that fits the most sprites within the given atlas dimensions and frame size. */
[[nodiscard]] std::expected<SheetLayout, std::string> compute_layout(int max_width, int max_height, int frame_w,
                                                                     int frame_h);

/**
 * All data required to execute a sprite packing operation. Built from CLI
 * options by from_options(), which loads sprites, resolves frame size,
 * computes the atlas layout, and calculates the number of sheets needed.
 */
struct PackData {
  std::vector<std::string> input_files;
  std::filesystem::path output_dir;
  std::string output_name;
  std::vector<Sprite> images;
  int frame_width{};
  int frame_height{};
  SheetLayout layout;
  int num_sheets{};

  /** Validates options, loads all sprites, and computes the full pack data. */
  [[nodiscard]] static std::expected<PackData, std::string> from_options(const Options &options);

  /** Blits sprites into atlas sheets and writes a PNG file for each sheet. */
  [[nodiscard]] std::expected<void, std::string> pack() const;

  /** Writes a JSON metadata file for each atlas sheet. */
  [[nodiscard]] std::expected<void, std::string> write_metadata() const;
};

#endif // SPRITE_PACKER_HPP
