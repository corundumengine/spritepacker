// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "sprite_packer.hpp"
#include <doctest/doctest.h>
#include <fstream>
#include <nlohmann/json.hpp>

TEST_SUITE("PackData::from_options") {
  TEST_CASE("non-existent source directory") {
    Options opts;
    opts.input = "/nonexistent/path/that/does/not/exist";
    opts.sheets = ".";
    opts.name = "test";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("Source directory does not exist") != std::string::npos);
  }

  TEST_CASE("non-existent output directory") {
    Options opts;
    opts.input = ".";
    opts.sheets = "/nonexistent/path/that/does/not/exist";
    opts.name = "test";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("Sheets directory does not exist") != std::string::npos);
  }

  TEST_CASE("no PNG files in source directory") {
    Options opts;
    opts.input = ".";
    opts.sheets = ".";
    opts.name = "test";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("No PNG files found") != std::string::npos);
  }

  TEST_CASE("invalid max size format is rejected") {
    Options opts;
    opts.input = ".";
    opts.sheets = ".";
    opts.name = "test";
    opts.max_size = "notasize";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("invalid padding value is rejected") {
    Options opts;
    opts.input = ".";
    opts.sheets = ".";
    opts.name = "test";
    opts.padding = "-1";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("must not be negative") != std::string::npos);
  }

  TEST_CASE("packs the fixture sprite end to end") {
    Options opts;
    opts.input = TEST_FIXTURES_DIR;
    opts.sheets = std::filesystem::temp_directory_path().string();
    opts.name = "e2e_test";
    auto result = PackData::from_options(opts);
    REQUIRE(result);
    CHECK_EQ(result->sprites.size(), 1u);
    CHECK_EQ(result->num_sheets, 1);
  }

  TEST_CASE("a single sprite larger than --max-size fails with a clear error, not a hang") {
    Options opts;
    opts.input = TEST_FIXTURES_DIR;
    opts.sheets = ".";
    opts.name = "test";
    // The fixture sprite is 1x1; a 1x1 max-size with nonzero padding can't fit it plus the gutter.
    opts.max_size = "1x1";
    opts.padding = "1";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("exceeds --max-size") != std::string::npos);
  }
}

namespace {

  // Builds a w x h RGBA sprite, fully transparent except for opaque pixels within
  // [ox, ox+ow) x [oy, oy+oh). Mirrors the helper in test_sprites.cpp.
  Sprite make_sprite(std::string name, int w, int h, int ox, int oy, int ow, int oh) {
    Sprite s;
    s.name = std::move(name);
    s.width = w;
    s.height = h;
    s.data.assign(static_cast<std::size_t>(w * h * 4), 0);
    for (int y = oy; y < oy + oh; ++y) {
      for (int x = ox; x < ox + ow; ++x) {
        const auto idx = static_cast<std::size_t>((y * w + x) * 4);
        s.data[idx + 3] = 255; // opaque
      }
    }
    return s;
  }

  // Builds a minimal PackData by hand (bypassing from_options's file I/O) so pack() and
  // write_metadata() can be tested directly against known sprite content.
  PackData build_pack_data(std::vector<Sprite> sprites) {
    PackData data;
    data.output_name = "test_output";

    for (auto &sprite : sprites) {
      const TrimRect trim = compute_trim(sprite);
      data.sprites.push_back(PackedSprite{
          .name = sprite.name,
          .sheet_index = 0,
          .x = 0, // overwritten below once placed
          .y = 0,
          .w = trim.w,
          .h = trim.h,
          .trim_x = trim.x,
          .trim_y = trim.y,
          .source_width = sprite.width,
          .source_height = sprite.height,
          .pivot_x = 0.5,
          .pivot_y = 1.0,
      });
      data.sprite_unique_index.push_back(static_cast<int>(data.unique_images.size()));
      data.unique_trims.push_back(trim);
      data.unique_images.push_back(std::move(sprite));
    }

    // Lay unique sprites out left to right with no overlap, sized to fit exactly.
    int x{0};
    int max_h{0};
    for (std::size_t i = 0; i < data.sprites.size(); ++i) {
      data.sprites[i].x = x;
      data.sprites[i].y = 0;
      x += data.sprites[i].w;
      max_h = std::max(max_h, data.sprites[i].h);
    }
    data.sheet_sizes.emplace_back(std::max(1, x), std::max(1, max_h));
    data.num_sheets = 1;
    return data;
  }

} // namespace

TEST_SUITE("write_metadata") {
  using json = nlohmann::json;

  TEST_CASE("writes schema_version, sheet dimensions, and per-sprite placement/trim/pivot fields") {
    PackData data = build_pack_data({make_sprite("hero_idle_0", 64, 64, 16, 32, 20, 32)});

    std::filesystem::path dir = std::filesystem::temp_directory_path() / "spritepacker_test_meta";
    std::filesystem::create_directories(dir);
    data.sheets_dir = dir;

    const auto result = data.write_metadata();
    REQUIRE(result.has_value());

    std::ifstream f(dir / "test_output.json");
    REQUIRE(f);
    const auto j = json::parse(f);
    CHECK_EQ(j["schema_version"], k_metadata_schema_version);
    CHECK(j.contains("path"));
    CHECK_EQ(j["width"], 20);
    CHECK_EQ(j["height"], 32);

    REQUIRE_EQ(j["sprites"].size(), 1u);
    const auto &sprite = j["sprites"][0];
    CHECK_EQ(sprite["name"], "hero_idle_0");
    CHECK_EQ(sprite["x"], 0);
    CHECK_EQ(sprite["y"], 0);
    CHECK_EQ(sprite["w"], 20);
    CHECK_EQ(sprite["h"], 32);
    CHECK_EQ(sprite["trim_x"], 16);
    CHECK_EQ(sprite["trim_y"], 32);
    CHECK_EQ(sprite["source_width"], 64);
    CHECK_EQ(sprite["source_height"], 64);
    CHECK_EQ(sprite["pivot_x"], 0.5);
    CHECK_EQ(sprite["pivot_y"], 1.0);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  TEST_CASE("only includes sprites belonging to the sheet being written") {
    PackData data = build_pack_data({make_sprite("a", 32, 32, 0, 0, 32, 32), make_sprite("b", 32, 32, 0, 0, 32, 32)});
    data.sprites[1].sheet_index = 1;
    data.sheet_sizes.emplace_back(32, 32);
    data.num_sheets = 2;

    std::filesystem::path dir = std::filesystem::temp_directory_path() / "spritepacker_test_meta_multi";
    std::filesystem::create_directories(dir);
    data.sheets_dir = dir;
    data.assets_dir = dir;

    const auto result = data.write_metadata();
    REQUIRE(result.has_value());

    std::ifstream f0(dir / "test_output-0.json");
    REQUIRE(f0);
    const auto j0 = json::parse(f0);
    REQUIRE_EQ(j0["sprites"].size(), 1u);
    CHECK_EQ(j0["sprites"][0]["name"], "a");

    std::ifstream f1(dir / "test_output-1.json");
    REQUIRE(f1);
    const auto j1 = json::parse(f1);
    REQUIRE_EQ(j1["sprites"].size(), 1u);
    CHECK_EQ(j1["sprites"][0]["name"], "b");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
}

TEST_SUITE("PackData::pack") {
  TEST_CASE("blits only the trimmed content of each sprite at its packed position") {
    PackData data = build_pack_data({make_sprite("hero", 64, 64, 16, 32, 20, 32)});

    std::filesystem::path dir = std::filesystem::temp_directory_path() / "spritepacker_test_pack";
    std::filesystem::create_directories(dir);
    data.sheets_dir = dir;
    data.assets_dir = dir;

    const auto result = data.pack();
    REQUIRE(result.has_value());

    Sprite loaded;
    REQUIRE(loaded.load(dir / "test_output.png"));
    CHECK_EQ(loaded.width, 20);
    CHECK_EQ(loaded.height, 32);
    // Every pixel of the packed sheet should be opaque, since the whole trimmed region was opaque.
    for (std::size_t i = 3; i < loaded.data.size(); i += 4)
      CHECK_EQ(loaded.data[i], 255);

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
}
