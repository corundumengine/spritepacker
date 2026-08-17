#include "sprite_packer.hpp"
#include "rect_packer.hpp"
#include "utils.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <lodepng.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <numeric>
#include <print>
#include <thread>
#include <unordered_map>

using json = nlohmann::json;

namespace fs = std::filesystem;

constexpr int k_default_max_atlas_dim{2048};
constexpr int k_default_padding{1};

namespace {

  std::expected<std::pair<int, int>, std::string> parse_size_or_default(const std::string &s, int dw, int dh) {
    if (s.empty())
      return std::pair{dw, dh};
    auto result = parse_size(s);
    if (!result)
      return std::unexpected(result.error());
    auto [w, h] = *result;
    return std::pair{w, h};
  }

  std::expected<int, std::string> parse_int_or_default(const std::string &s, std::string_view option_name, int def) {
    if (s.empty())
      return def;
    return parse_int(s, option_name);
  }

  /** Decodes every file in @p files concurrently. Sprite order in the result matches @p files. */
  std::expected<std::vector<Sprite>, std::string> load_sprites_parallel(const std::vector<std::string> &files) {
    std::vector<Sprite> images(files.size());
    std::atomic<std::size_t> next_index{0};
    std::mutex error_mutex;
    std::string first_error;

    const unsigned worker_count{
        std::max(1u, std::min(std::thread::hardware_concurrency(), static_cast<unsigned>(files.size())))};
    {
      std::vector<std::jthread> workers;
      workers.reserve(worker_count);
      for (unsigned w = 0; w < worker_count; ++w) {
        workers.emplace_back([&files, &images, &next_index, &error_mutex, &first_error] {
          for (;;) {
            const std::size_t i{next_index.fetch_add(1)};
            if (i >= files.size())
              break;
            if (auto result = images[i].load(files[i]); !result) {
              const std::lock_guard lock{error_mutex};
              if (first_error.empty())
                first_error = std::format("Could not load sprite: {} ({})", files[i], result.error());
              continue;
            }
            if (!images[i].is_valid()) {
              const std::lock_guard lock{error_mutex};
              if (first_error.empty())
                first_error = std::format("Invalid sprite (0x0) in: {}", files[i]);
            }
          }
        });
      }
    } // jthreads join here

    if (!first_error.empty())
      return std::unexpected(first_error);
    return images;
  }

  int next_power_of_two(int v) noexcept {
    if (v <= 0)
      return 1;
    return static_cast<int>(std::bit_ceil(static_cast<unsigned>(v)));
  }

  std::filesystem::path sheet_path(const PackData &data, int idx, std::string_view ext) {
    const std::string suffix{(data.num_sheets > 1) ? std::format("-{}", idx) : ""};
    const auto &base = (ext == ".png") ? data.assets_dir : data.sheets_dir;
    return base / std::format("{}{}{}", data.output_name, suffix, ext);
  }

  void blit_trimmed(const Sprite &sprite, const TrimRect &trim, Atlas &atlas, int dest_x, int dest_y) {
    assert(dest_x >= 0 && dest_y >= 0);
    assert(dest_x + trim.w <= atlas.width && dest_y + trim.h <= atlas.height);
    const auto row_bytes = static_cast<std::size_t>(trim.w) * k_bytes_per_pixel;
    for (int row = 0; row < trim.h; ++row) {
      const auto src = static_cast<std::size_t>((trim.y + row) * sprite.width + trim.x) * k_bytes_per_pixel;
      const auto dst = static_cast<std::size_t>((dest_y + row) * atlas.width + dest_x) * k_bytes_per_pixel;
      std::copy_n(sprite.data.data() + src, row_bytes, atlas.data.data() + dst);
    }
  }

} // namespace

std::expected<PackData, std::string> PackData::from_options(const Options &options) {
  const fs::path source_dir{options.input};
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir))
    return std::unexpected(std::format("Source directory does not exist: {}", options.input));

  const fs::path sheets_dir{options.sheets};
  if (!fs::exists(sheets_dir) || !fs::is_directory(sheets_dir))
    return std::unexpected(std::format("Sheets directory does not exist: {}", options.sheets));

  fs::path assets_dir = sheets_dir;
  if (!options.assets.empty()) {
    assets_dir = fs::path{options.assets};
    if (!fs::exists(assets_dir) || !fs::is_directory(assets_dir))
      return std::unexpected(std::format("Assets directory does not exist: {}", options.assets));
  }

  const auto max_size = parse_size_or_default(options.max_size, k_default_max_atlas_dim, k_default_max_atlas_dim);
  if (!max_size)
    return std::unexpected(max_size.error());
  const auto [max_w, max_h] = *max_size;

  const auto padding_result = parse_int_or_default(options.padding, "--padding", k_default_padding);
  if (!padding_result)
    return std::unexpected(padding_result.error());
  const int padding{*padding_result};

  auto input_files = resolve_input_files(source_dir, options.files);
  if (!input_files)
    return std::unexpected(input_files.error());

  std::println("Found {} sprite file(s):", input_files->size());
  for (const auto &f : *input_files)
    std::println("  {}", fs::path(f).filename().string());

  if (options.validate_animations) {
    if (auto result = validate_animation_naming(*input_files); !result)
      return std::unexpected(result.error());
  }

  auto images = load_sprites_parallel(*input_files);
  if (!images)
    return std::unexpected(images.error());

  std::vector<TrimRect> trims(images->size());
  std::vector<std::string> names(images->size());
  for (std::size_t i = 0; i < images->size(); ++i) {
    trims[i] = compute_trim((*images)[i]);
    names[i] = (*images)[i].name;
    if (trims[i].w + padding > max_w || trims[i].h + padding > max_h)
      return std::unexpected(std::format(
          "Sprite '{}' ({}x{} after trimming) plus padding ({}) exceeds --max-size {}x{}; increase --max-size, "
          "reduce --padding, or split the sprite",
          names[i], trims[i].w, trims[i].h, padding, max_w, max_h));
  }

  // Deduplicate by trimmed pixel content: identical frames reused across states (e.g. an idle
  // frame shared with an attack-recovery frame) are packed once and every referencing name in the
  // exported metadata points at that single packed instance.
  std::vector<Sprite> unique_images;
  std::vector<TrimRect> unique_trims;
  std::vector<int> sprite_unique_index(images->size());
  std::unordered_map<std::size_t, std::vector<int>> hash_buckets;

  for (std::size_t i = 0; i < images->size(); ++i) {
    const std::size_t hash{compute_content_hash((*images)[i], trims[i])};
    int found{-1};
    if (auto it = hash_buckets.find(hash); it != hash_buckets.end()) {
      for (const int idx : it->second) {
        if (trimmed_content_equal((*images)[i], trims[i], unique_images[static_cast<std::size_t>(idx)],
                                  unique_trims[static_cast<std::size_t>(idx)])) {
          found = idx;
          break;
        }
      }
    }
    if (found >= 0) {
      sprite_unique_index[i] = found;
    } else {
      unique_trims.push_back(trims[i]);
      unique_images.push_back(std::move((*images)[i]));
      const int new_idx{static_cast<int>(unique_images.size()) - 1};
      hash_buckets[hash].push_back(new_idx);
      sprite_unique_index[i] = new_idx;
    }
  }

  // Pack largest-first: placing big sprites while free space is least fragmented tends to yield a
  // tighter overall layout than packing in arbitrary input order.
  std::vector<int> pack_order(unique_images.size());
  std::iota(pack_order.begin(), pack_order.end(), 0);
  std::ranges::sort(pack_order, [&unique_trims](int a, int b) {
    const auto &ta = unique_trims[static_cast<std::size_t>(a)];
    const auto &tb = unique_trims[static_cast<std::size_t>(b)];
    return std::max(ta.w, ta.h) > std::max(tb.w, tb.h);
  });

  struct Placement {
    int sheet_index{-1};
    PackedRect rect;
  };

  std::vector<Placement> placements(unique_images.size());
  std::vector<MaxRectsPacker> packers;
  std::vector<std::pair<int, int>> used_bounds;

  for (const int idx : pack_order) {
    const auto &trim = unique_trims[static_cast<std::size_t>(idx)];
    const int req_w{trim.w + padding};
    const int req_h{trim.h + padding};

    std::optional<PackedRect> rect;
    if (!packers.empty())
      rect = packers.back().insert(req_w, req_h);

    int sheet_index{static_cast<int>(packers.size()) - 1};
    if (!rect) {
      packers.emplace_back(max_w, max_h);
      used_bounds.emplace_back(0, 0);
      sheet_index = static_cast<int>(packers.size()) - 1;
      rect = packers[static_cast<std::size_t>(sheet_index)].insert(req_w, req_h);
      if (!rect)
        return std::unexpected(std::format("Internal error: sprite '{}' failed to pack into an empty sheet",
                                           names[static_cast<std::size_t>(idx)]));
    }

    placements[static_cast<std::size_t>(idx)] = Placement{sheet_index, PackedRect{rect->x, rect->y, trim.w, trim.h}};
    auto &bounds = used_bounds[static_cast<std::size_t>(sheet_index)];
    bounds.first = std::max(bounds.first, rect->x + trim.w);
    bounds.second = std::max(bounds.second, rect->y + trim.h);
  }

  std::vector<std::pair<int, int>> sheet_sizes;
  sheet_sizes.reserve(used_bounds.size());
  for (const auto &[w, h] : used_bounds) {
    int sw{std::max(1, w)};
    int sh{std::max(1, h)};
    if (options.pot) {
      sw = next_power_of_two(sw);
      sh = next_power_of_two(sh);
    }
    sheet_sizes.emplace_back(sw, sh);
  }

  const auto pivot = resolve_pivot(options.pivot);
  if (!pivot)
    return std::unexpected(pivot.error());

  std::vector<PackedSprite> sprites;
  sprites.reserve(images->size());
  for (std::size_t i = 0; i < images->size(); ++i) {
    const int uidx{sprite_unique_index[i]};
    const auto &placement = placements[static_cast<std::size_t>(uidx)];
    const auto &unique_sprite = unique_images[static_cast<std::size_t>(uidx)];
    const auto &unique_trim = unique_trims[static_cast<std::size_t>(uidx)];

    sprites.push_back(PackedSprite{
        .name = names[i],
        .sheet_index = placement.sheet_index,
        .x = placement.rect.x,
        .y = placement.rect.y,
        .w = placement.rect.w,
        .h = placement.rect.h,
        .trim_x = unique_trim.x,
        .trim_y = unique_trim.y,
        .source_width = unique_sprite.width,
        .source_height = unique_sprite.height,
        .pivot_x = pivot->x,
        .pivot_y = pivot->y,
    });
  }

  const int num_sheets{static_cast<int>(packers.size())};
  const std::size_t duplicate_count{images->size() - unique_images.size()};
  std::println("Packed {} unique sprite(s) ({} duplicate(s) deduplicated) into {} sheet(s).", unique_images.size(),
               duplicate_count, num_sheets);

  return PackData{
      .sheets_dir = sheets_dir,
      .assets_dir = std::move(assets_dir),
      .output_name = options.name,
      .full_canvas_pivot = pivot->full_canvas,
      .unique_images = std::move(unique_images),
      .unique_trims = std::move(unique_trims),
      .sprites = std::move(sprites),
      .sprite_unique_index = std::move(sprite_unique_index),
      .sheet_sizes = std::move(sheet_sizes),
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

std::expected<void, std::string> PackData::pack() const {
  std::vector<int> representative(unique_images.size(), -1);
  for (std::size_t i = 0; i < sprites.size(); ++i) {
    const int u{sprite_unique_index[i]};
    if (representative[static_cast<std::size_t>(u)] < 0)
      representative[static_cast<std::size_t>(u)] = static_cast<int>(i);
  }

  for (int sheet_idx = 0; sheet_idx < num_sheets; ++sheet_idx) {
    const auto [atlas_w, atlas_h] = sheet_sizes[static_cast<std::size_t>(sheet_idx)];

    Atlas atlas{
        .data = std::vector<uint8_t>(static_cast<std::size_t>(atlas_w * atlas_h * k_bytes_per_pixel), 0),
        .width = atlas_w,
        .height = atlas_h,
    };

    for (std::size_t u = 0; u < unique_images.size(); ++u) {
      const int rep{representative[u]};
      if (rep < 0)
        continue;
      const PackedSprite &ps = sprites[static_cast<std::size_t>(rep)];
      if (ps.sheet_index != sheet_idx)
        continue;
      blit_trimmed(unique_images[u], unique_trims[u], atlas, ps.x, ps.y);
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
    const auto [atlas_w, atlas_h] = sheet_sizes[static_cast<std::size_t>(sheet_idx)];

    json metadata;
    metadata["schema_version"] = k_metadata_schema_version;
    metadata["path"] = png_path.string();
    metadata["width"] = atlas_w;
    metadata["height"] = atlas_h;
    if (full_canvas_pivot)
      metadata["pivot_basis"] = "full"; // pivots are fractions of the full source canvas, y from the bottom

    json sprite_array = json::array();
    for (const auto &sprite : sprites) {
      if (sprite.sheet_index != sheet_idx)
        continue;
      sprite_array.push_back({
          {"name", sprite.name},
          {"x", sprite.x},
          {"y", sprite.y},
          {"w", sprite.w},
          {"h", sprite.h},
          {"trim_x", sprite.trim_x},
          {"trim_y", sprite.trim_y},
          {"source_width", sprite.source_width},
          {"source_height", sprite.source_height},
          {"pivot_x", sprite.pivot_x},
          {"pivot_y", sprite.pivot_y},
      });
    }
    metadata["sprites"] = std::move(sprite_array);

    std::ofstream json_file(json_path);
    if (!json_file)
      return std::unexpected(std::format("Failed to open JSON file: {}", json_path.string()));

    json_file << metadata.dump(2);
    if (json_file.fail())
      return std::unexpected(std::format("Failed to write JSON file: {}", json_path.string()));
  }
  return {};
}
