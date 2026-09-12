// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "sprites.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <format>
#include <lodepng.h>
#include <string>

std::expected<void, std::string> Sprite::load(const std::filesystem::path &path) {
  unsigned w{};
  unsigned h{};
  const unsigned err{lodepng::decode(data, w, h, path.string())};
  if (err != 0u)
    return std::unexpected(std::format("Failed to load PNG: {} ({})", path.string(), lodepng_error_text(err)));
  width = static_cast<int>(w);
  height = static_cast<int>(h);
  name = path.stem().string();
  return {};
}

TrimRect compute_trim(const Sprite &sprite) noexcept {
  int min_x{sprite.width};
  int min_y{sprite.height};
  int max_x{-1};
  int max_y{-1};
  for (int y = 0; y < sprite.height; ++y) {
    for (int x = 0; x < sprite.width; ++x) {
      const auto idx = static_cast<std::size_t>((y * sprite.width) + x) * k_bytes_per_pixel;
      if (sprite.data[idx + 3] > 0) { // alpha channel
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
      }
    }
  }
  if (max_x < 0) // fully transparent — nothing to trim to, report the whole frame
    return {.x = 0, .y = 0, .w = sprite.width, .h = sprite.height};
  return {.x = min_x, .y = min_y, .w = max_x - min_x + 1, .h = max_y - min_y + 1};
}

std::size_t compute_content_hash(const Sprite &sprite, const TrimRect &trim) noexcept {
  // FNV-1a over the trimmed pixel region plus its dimensions, so sprites with identical visible
  // pixels but different trimmed sizes (a degenerate case) don't collide.
  std::size_t hash{0xcbf29ce484222325ULL};
  constexpr std::size_t k_prime{0x100000001b3ULL};
  const auto mix = [&hash](std::size_t v) {
    hash ^= v;
    hash *= k_prime;
  };
  mix(static_cast<std::size_t>(trim.w));
  mix(static_cast<std::size_t>(trim.h));
  for (int y = 0; y < trim.h; ++y) {
    const auto row_start = static_cast<std::size_t>(((trim.y + y) * sprite.width) + trim.x) * k_bytes_per_pixel;
    const auto row_bytes = static_cast<std::size_t>(trim.w) * k_bytes_per_pixel;
    for (std::size_t i = 0; i < row_bytes; ++i)
      mix(sprite.data[row_start + i]);
  }
  return hash;
}

bool trimmed_content_equal(const Sprite &a, const TrimRect &trim_a, const Sprite &b, const TrimRect &trim_b) noexcept {
  if (trim_a.w != trim_b.w || trim_a.h != trim_b.h)
    return false;
  for (int y = 0; y < trim_a.h; ++y) {
    const auto a_row = static_cast<std::size_t>(((trim_a.y + y) * a.width) + trim_a.x) * k_bytes_per_pixel;
    const auto b_row = static_cast<std::size_t>(((trim_b.y + y) * b.width) + trim_b.x) * k_bytes_per_pixel;
    const auto row_bytes = static_cast<std::size_t>(trim_a.w) * k_bytes_per_pixel;
    if (!std::equal(a.data.begin() + static_cast<std::ptrdiff_t>(a_row),
                    a.data.begin() + static_cast<std::ptrdiff_t>(a_row + row_bytes),
                    b.data.begin() + static_cast<std::ptrdiff_t>(b_row)))
      return false;
  }
  return true;
}
