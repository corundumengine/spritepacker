#include "sprites.hpp"

#include <algorithm>
#include <format>
#include <lodepng.h>

namespace {
  constexpr int k_bytes_per_pixel{4};
}

std::expected<void, std::string> Sprite::load(const std::filesystem::path &path) {
  unsigned w{}, h{};
  const unsigned err{lodepng::decode(data, w, h, path.string())};
  if (err)
    return std::unexpected(std::format("Failed to load PNG: {} ({})", path.string(), lodepng_error_text(err)));
  width = static_cast<int>(w);
  height = static_cast<int>(h);
  return {};
}

TrimRect compute_trim(const Sprite &sprite) noexcept {
  int min_x{sprite.width}, min_y{sprite.height}, max_x{-1}, max_y{-1};
  for (int y = 0; y < sprite.height; ++y) {
    for (int x = 0; x < sprite.width; ++x) {
      const auto idx = static_cast<std::size_t>(y * sprite.width + x) * k_bytes_per_pixel;
      if (sprite.data[idx + 3] > 0) { // alpha channel
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
      }
    }
  }
  if (max_x < 0) // fully transparent — nothing to trim to, report the whole frame
    return {0, 0, sprite.width, sprite.height};
  return {min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
}
