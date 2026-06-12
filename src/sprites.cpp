#include "sprites.hpp"

#include <format>
#include <lodepng.h>

std::expected<void, std::string> Sprite::load(const std::filesystem::path &path) {
  unsigned w{}, h{};
  const unsigned err{lodepng::decode(data, w, h, path.string())};
  if (err)
    return std::unexpected(std::format("Failed to load PNG: {} ({})", path.string(), lodepng_error_text(err)));
  width = static_cast<int>(w);
  height = static_cast<int>(h);
  return {};
}
