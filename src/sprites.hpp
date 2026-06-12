#ifndef SPRITES_HPP
#define SPRITES_HPP

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

/** A single source PNG sprite loaded into memory as raw RGBA pixels. */
struct Sprite {
  std::vector<uint8_t> data;
  int width{}, height{};

  /** Returns true if the sprite has pixel data and positive dimensions. */
  [[nodiscard]] bool is_valid() const noexcept {
    return !data.empty() && width > 0 && height > 0;
  }

  /** Decodes a PNG file into this sprite's data, width, and height. */
  [[nodiscard]] std::expected<void, std::string> load(const std::filesystem::path &path);
};

/** Grid geometry for a single sprite sheet: how many columns and rows fit. */
struct SheetLayout {
  int cols;
  int rows;

  [[nodiscard]] constexpr int sprites_per_sheet() const noexcept {
    return cols * rows;
  }
};

#endif // SPRITES_HPP
