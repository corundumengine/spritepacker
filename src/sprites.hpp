#ifndef SPRITES_HPP
#define SPRITES_HPP

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

/** Number of bytes per pixel in the RGBA format used throughout. */
inline constexpr int k_bytes_per_pixel{4};

/** A single source PNG sprite loaded into memory as raw RGBA pixels. */
struct Sprite {
  std::vector<uint8_t> data;
  std::string name; // filename stem, used as the sprite's ID in exported metadata
  int width{}, height{};

  /** Returns true if the sprite has pixel data and positive dimensions. */
  [[nodiscard]] bool is_valid() const noexcept {
    return !data.empty() && width > 0 && height > 0;
  }

  /** Decodes a PNG file into this sprite's data, width, and height. */
  [[nodiscard]] std::expected<void, std::string> load(const std::filesystem::path &path);
};

/** The tightest bounding box of a sprite's non-transparent (alpha > 0) pixels, in that sprite's own
 *  pixel coordinates (top-left origin). A fully-transparent sprite returns {0, 0, width, height} —
 *  the whole frame, since there's no visible content to trim to. */
struct TrimRect {
  int x{}, y{}, w{}, h{};
};

/** Scans @p sprite's alpha channel for its tightest non-transparent bounding box. */
[[nodiscard]] TrimRect compute_trim(const Sprite &sprite) noexcept;

/**
 * Hashes the pixel content of @p sprite within @p trim (i.e. after trimming away transparent
 * padding), so two sprites with identical visible content but different surrounding padding or
 * canvas size still hash equal. Used to detect duplicate frames before packing.
 */
[[nodiscard]] std::size_t compute_content_hash(const Sprite &sprite, const TrimRect &trim) noexcept;

/** True if @p a and @p b have pixel-identical content within their respective trim rects. */
[[nodiscard]] bool trimmed_content_equal(const Sprite &a, const TrimRect &trim_a, const Sprite &b,
                                         const TrimRect &trim_b) noexcept;

#endif // SPRITES_HPP
