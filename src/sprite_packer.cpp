#include "sprite_packer.hpp"
#include "utils.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
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
        return std::unexpected(std::format("Could not load sprite: {} ({})", file, result.error()));
      if (!img.is_valid())
        return std::unexpected(std::format("Invalid sprite (0x0) in: {}", file));
      if (img.width != img.height)
        std::println(stderr, "Warning: Sprite is not square: {} ({}x{})", file, img.width, img.height);
      max_w = std::max(max_w, img.width);
      max_h = std::max(max_h, img.height);
      images.push_back(std::move(img));
    }
    return LoadedSprites{std::move(images), max_w, max_h};
  }

  std::filesystem::path sheet_path(const PackData &data, int idx, std::string_view ext) {
    const std::string suffix{(data.num_sheets > 1) ? std::format("-{}", idx) : ""};
    const auto &base = (ext == ".png") ? data.assets_dir : data.sheets_dir;
    return base / std::format("{}{}{}", data.output_name, suffix, ext);
  }

  void blit_sprite(const Sprite &sprite, Atlas &atlas, int dest_x, int dest_y) {
    for (int sy = 0; sy < sprite.height; ++sy) {
      const auto src = static_cast<std::size_t>(sy * sprite.width * 4);
      const auto dst = static_cast<std::size_t>(((dest_y + sy) * atlas.width + dest_x) * 4);
      std::copy_n(sprite.data.data() + src, sprite.width * 4, atlas.data.data() + dst);
    }
  }

} // namespace

CropRect find_visible_bounds(const Sprite &sprite) noexcept {
  if (!sprite.is_valid())
    return {};

  int min_x = sprite.width, min_y = sprite.height;
  int max_x = -1, max_y = -1;

  for (int y = 0; y < sprite.height; ++y) {
    for (int x = 0; x < sprite.width; ++x) {
      if (sprite.data[static_cast<std::size_t>((y * sprite.width + x) * 4 + 3)] > 0) {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
      }
    }
  }

  if (max_x < 0)
    return {};

  return {min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
}

std::expected<PackData, std::string> PackData::from_options(const Options &options) {
  const fs::path source_dir{options.input};
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    return std::unexpected(std::format("Source directory does not exist: {}", options.input));
  }

  const fs::path sheets_dir{options.sheets};
  if (!fs::exists(sheets_dir) || !fs::is_directory(sheets_dir)) {
    return std::unexpected(std::format("Sheets directory does not exist: {}", options.sheets));
  }

  fs::path assets_dir = sheets_dir;
  if (!options.assets.empty()) {
    assets_dir = fs::path{options.assets};
    if (!fs::exists(assets_dir) || !fs::is_directory(assets_dir)) {
      return std::unexpected(std::format("Assets directory does not exist: {}", options.assets));
    }
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

  std::vector<CropRect> crops;
  int usable_w{};
  int usable_h{};
  int max_cw{};
  int max_ch{};

  if (options.character) {
    usable_w = loaded->max_width;
    usable_h = loaded->max_height;
  } else {
    crops.resize(loaded->images.size());
    for (std::size_t i = 0; i < loaded->images.size(); ++i) {
      crops[i] = find_visible_bounds(loaded->images[i]);
      if (crops[i].w == 0 || crops[i].h == 0) {
        std::println(stderr, "Warning: fully transparent sprite skipped");
        crops[i] = {0, 0, 1, 1};
      }
      CropRect &c = crops[i];
      const int left = std::max(0, c.x - options.margin);
      const int top = std::max(0, c.y - options.margin);
      const int right = std::min(loaded->images[i].width - 1, c.x + c.w - 1 + options.margin);
      const int bottom = std::min(loaded->images[i].height - 1, c.y + c.h - 1 + options.margin);
      c.x = left;
      c.y = top;
      c.w = right - left + 1;
      c.h = bottom - top + 1;

      max_cw = std::max(max_cw, c.w);
      max_ch = std::max(max_ch, c.h);
    }
    usable_w = max_cw;
    usable_h = max_ch;
  }

  const int frame_w{frame_hint_w ? frame_hint_w : usable_w};
  const int frame_h{frame_hint_h ? frame_hint_h : usable_h};

  if (frame_w == 0 || frame_h == 0)
    return std::unexpected(std::format("Invalid frame size: {}x{}", frame_w, frame_h));

  std::string_view frame_suffix;
  if (!frame_hint_w)
    frame_suffix = options.character ? " (auto-detected)" : " (auto-detected from crop)";
  std::println("Frame size: {}x{}{}", frame_w, frame_h, frame_suffix);

  if (options.character) {
    for (const auto &sprite : loaded->images) {
      if (sprite.width > frame_w || sprite.height > frame_h)
        return std::unexpected(
            std::format("Sprite {}x{} exceeds frame size {}x{}", sprite.width, sprite.height, frame_w, frame_h));
    }
  } else {
    if (max_cw > frame_w || max_ch > frame_h)
      return std::unexpected(std::format("--size {}x{} is too small for the largest cropped sprite ({}x{})", frame_w,
                                         frame_h, max_cw, max_ch));
  }

  const auto layout = compute_layout(max_w, max_h, frame_w, frame_h);
  if (!layout)
    return std::unexpected(layout.error());

  std::println("Grid: {}x{} ({} sprites per sheet)", layout->cols, layout->rows, layout->sprites_per_sheet());

  const auto num_sheets_val{(loaded->images.size() + static_cast<std::size_t>(layout->sprites_per_sheet()) - 1) /
                            static_cast<std::size_t>(layout->sprites_per_sheet())};
  if (num_sheets_val > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    return std::unexpected(std::format("Too many sheets required ({})", num_sheets_val));
  const int num_sheets{static_cast<int>(num_sheets_val)};

  return PackData{
      .input_files = std::move(*input_files),
      .sheets_dir = sheets_dir,
      .assets_dir = std::move(assets_dir),
      .output_name = options.name,
      .images = std::move(loaded->images),
      .frame_width = frame_w,
      .frame_height = frame_h,
      .layout = *layout,
      .num_sheets = num_sheets,
      .character = options.character,
      .margin = options.margin,
      .crops = std::move(crops),
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
        const Sprite &sprite = images[static_cast<std::size_t>(sprite_idx)];
        const int cell_x = col * frame_width;
        const int cell_y = row * frame_height;

        if (character) {
          blit_sprite(sprite, atlas, cell_x, cell_y);
        } else {
          const CropRect &crop = crops[static_cast<std::size_t>(sprite_idx)];
          const int blit_x = cell_x + (frame_width - crop.w) / 2;
          const int blit_y = cell_y + frame_height - crop.h;
          for (int sr = 0; sr < crop.h; ++sr) {
            const auto src = static_cast<std::size_t>(((crop.y + sr) * sprite.width + crop.x) * 4);
            const auto dst = static_cast<std::size_t>(((blit_y + sr) * atlas.width + blit_x) * 4);
            std::copy_n(sprite.data.data() + src, static_cast<std::size_t>(crop.w) * 4, atlas.data.data() + dst);
          }
        }
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
    const std::filesystem::path png_path = sheet_path(*this, sheet_idx, ".png");
    const std::filesystem::path json_path = sheet_path(*this, sheet_idx, ".json");

    json metadata;
    metadata["path"] = png_path.string();
    metadata["columns"] = layout.cols;
    metadata["rows"] = layout.rows;

    if (character) {
      metadata["frame_width"] = frame_width;
      metadata["frame_height"] = frame_height;
      metadata["offset_x"] = 0;
      metadata["offset_y"] = 0;
      metadata["spacing_x"] = 0;
      metadata["spacing_y"] = 0;
    } else {
      metadata["tile_width"] = frame_width;
      metadata["tile_height"] = frame_height;
      metadata["pivot_x"] = 0.5f;
      metadata["pivot_y"] = 0.0f;
    }

    std::ofstream json_file(json_path);
    if (!json_file)
      return std::unexpected(std::format("Failed to open JSON file: {}", json_path.string()));

    json_file << metadata.dump(2);
    if (json_file.fail())
      return std::unexpected(std::format("Failed to write JSON file: {}", json_path.string()));
  }
  return {};
}
