#include "sprite_packer.hpp"
#include "utils.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <lodepng.h>
#include <nlohmann/json.hpp>
#include <print>

using json = nlohmann::json;

namespace fs = std::filesystem;

namespace {

  std::expected<std::pair<int, int>, std::string> parse_or_default(const std::string &s, int dw, int dh) {
    if (s.empty())
      return std::pair{dw, dh};
    auto result = parse_size(s);
    if (!result)
      return std::unexpected(result.error());
    auto [w, h] = *result;
    return std::pair{w, h};
  }

  struct LoadedSprites {
    std::vector<Sprite> images;
    int max_width{}, max_height{};
  };

  std::expected<LoadedSprites, std::string> load_sprites(const std::vector<std::string> &files) {
    std::vector<Sprite> images;
    int max_w{}, max_h{};
    for (const auto &file : files) {
      Sprite img;
      if (auto result = img.load(file); !result)
        return std::unexpected(std::format("Could not load sprite: {}", file));
      if (!img.is_valid())
        return std::unexpected(std::format("Invalid sprite (0x0) in: {}", file));
      if (img.width != img.height)
        std::println(std::cerr, "Warning: Sprite is not square: {} ({}x{})", file, img.width, img.height);
      max_w = std::max(max_w, img.width);
      max_h = std::max(max_h, img.height);
      images.push_back(std::move(img));
    }
    return LoadedSprites{std::move(images), max_w, max_h};
  }

  std::filesystem::path sheet_path(const PackData &data, int idx, std::string_view ext) {
    const std::string suffix{(data.num_sheets > 1) ? std::format("-{}", idx) : ""};
    return data.output_dir / std::format("{}{}{}", data.output_name, suffix, ext);
  }

  void blit_sprite(const Sprite &sprite, Atlas &atlas, int dest_x, int dest_y) {
    for (int sy = 0; sy < sprite.height; ++sy) {
      const auto src = static_cast<std::size_t>(sy * sprite.width * 4);
      const auto dst = static_cast<std::size_t>(((dest_y + sy) * atlas.width + dest_x) * 4);
      std::copy_n(sprite.data.data() + src, sprite.width * 4, atlas.data.data() + dst);
    }
  }

} // namespace

std::expected<PackData, std::string> PackData::from_options(const Options &options) {
  const fs::path source_dir{options.input};
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    return std::unexpected(std::format("Source directory does not exist: {}", options.input));
  }

  const fs::path output_dir{options.output};
  if (!fs::exists(output_dir) || !fs::is_directory(output_dir)) {
    return std::unexpected(std::format("Output directory does not exist: {}", options.output));
  }

  const auto frame_size = parse_or_default(options.size, 0, 0);
  if (!frame_size)
    return std::unexpected(frame_size.error());
  const auto [frame_hint_w, frame_hint_h] = *frame_size;

  const auto max_size = parse_or_default(options.max_size, 2048, 2048);
  if (!max_size)
    return std::unexpected(max_size.error());
  const auto [max_w, max_h] = *max_size;

  auto input_files = resolve_input_files(source_dir, options.files);
  if (!input_files)
    return std::unexpected(input_files.error());

  std::println("Found {} sprite file(s):", input_files->size());
  for (const auto &f : *input_files)
    std::println("  {}", fs::path(f).filename().string());

  const auto loaded = load_sprites(*input_files);
  if (!loaded)
    return std::unexpected(loaded.error());

  const int frame_w{frame_hint_w ? frame_hint_w : loaded->max_width};
  const int frame_h{frame_hint_h ? frame_hint_h : loaded->max_height};

  if (frame_w == 0 || frame_h == 0)
    return std::unexpected(std::format("Invalid frame size: {}x{}", frame_w, frame_h));

  std::println("Frame size: {}x{}{}", frame_w, frame_h, frame_hint_w ? "" : " (auto-detected)");

  for (const auto &sprite : loaded->images) {
    if (sprite.width > frame_w || sprite.height > frame_h)
      return std::unexpected(
          std::format("Sprite {}x{} exceeds frame size {}x{}", sprite.width, sprite.height, frame_w, frame_h));
  }

  const auto layout = compute_layout(max_w, max_h, frame_w, frame_h);
  if (!layout)
    return std::unexpected(layout.error());

  std::println("Grid: {}x{} ({} sprites per sheet)", layout->cols, layout->rows, layout->sprites_per_sheet());

  const int num_sheets{
      static_cast<int>((loaded->images.size() + layout->sprites_per_sheet() - 1) / layout->sprites_per_sheet())};

  return PackData{
      .input_files = std::move(*input_files),
      .output_dir = output_dir,
      .output_name = options.name,
      .images = std::move(loaded->images),
      .frame_width = frame_w,
      .frame_height = frame_h,
      .layout = *layout,
      .num_sheets = num_sheets,
  };
}

std::expected<void, std::string> Atlas::write(const std::filesystem::path &path) const {
  const unsigned err{lodepng::encode(path.string(), data, static_cast<unsigned>(width), static_cast<unsigned>(height))};
  if (err) {
    return std::unexpected(std::format("Failed to write PNG: {} ({})", path.string(), lodepng_error_text(err)));
  }
  return {};
}

std::expected<SheetLayout, std::string> compute_layout(int max_width, int max_height, int frame_w, int frame_h) {
  if (frame_w <= 0 || frame_h <= 0) {
    return std::unexpected(std::format("Invalid frame size: {}x{}", frame_w, frame_h));
  }
  const int cols{std::max(1, max_width / frame_w)};
  const int rows{std::max(1, max_height / frame_h)};
  return SheetLayout{cols, rows};
}

std::expected<void, std::string> PackData::pack() const {
  for (int sheet_idx = 0; sheet_idx < num_sheets; ++sheet_idx) {
    const int atlas_w{layout.cols * frame_width};
    const int atlas_h{layout.rows * frame_height};

    Atlas atlas{
        .data = std::vector<uint8_t>(static_cast<std::size_t>(atlas_w * atlas_h * 4), 0),
        .width = atlas_w,
        .height = atlas_h,
    };

    int sprite_idx{sheet_idx * layout.sprites_per_sheet()};
    for (int row = 0; row < layout.rows && sprite_idx < static_cast<int>(images.size()); ++row) {
      for (int col = 0; col < layout.cols && sprite_idx < static_cast<int>(images.size()); ++col) {
        blit_sprite(images[static_cast<std::size_t>(sprite_idx)], atlas, col * frame_width, row * frame_height);
        ++sprite_idx;
      }
    }

    if (auto result = atlas.write(sheet_path(*this, sheet_idx, ".png")); !result)
      return std::unexpected(result.error());
  }

  return {};
}

std::expected<void, std::string> PackData::write_metadata() const {
  for (int sheet_idx = 0; sheet_idx < num_sheets; ++sheet_idx) {
    const std::string sheet_id{(num_sheets > 1) ? std::format("{}-{}", output_name, sheet_idx) : output_name};
    const auto png_path = sheet_path(*this, sheet_idx, ".png");
    const auto json_path = sheet_path(*this, sheet_idx, ".json");

    const json metadata = {
        {"id", sheet_id},
        {"path", png_path.string()},
        {"tile_width", frame_width},
        {"tile_height", frame_height},
        {"columns", layout.cols},
        {"rows", layout.rows},
        {"offset_x", 0},
        {"offset_y", 0},
        {"spacing_x", 0},
        {"spacing_y", 0},
    };

    std::ofstream json_file(json_path);
    if (!json_file)
      return std::unexpected(std::format("Failed to open JSON file: {}", json_path.string()));

    json_file << metadata.dump(2);
  }
  return {};
}
