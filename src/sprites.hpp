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

/** The tightest bounding box of a sprite's non-transparent (alpha > 0) pixels, in that sprite's own
 *  pixel coordinates (top-left origin). A fully-transparent sprite returns {0, 0, width, height} —
 *  the whole frame, since there's no visible content to trim to. */
struct TrimRect {
  int x{}, y{}, w{}, h{};
};

/** Scans @p sprite's alpha channel for its tightest non-transparent bounding box. */
[[nodiscard]] TrimRect compute_trim(const Sprite &sprite) noexcept;

#endif // SPRITES_HPP
