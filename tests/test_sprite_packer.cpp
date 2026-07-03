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

  TEST_CASE("invalid size format is rejected") {
    Options opts;
    opts.input = ".";
    opts.sheets = ".";
    opts.name = "test";
    opts.size = "notasize";
    auto result = PackData::from_options(opts);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
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
}

TEST_SUITE("compute_layout") {
  TEST_CASE("4x4 grid") {
    auto result = compute_layout(256, 256, 64, 64);
    CHECK(result);
    CHECK_EQ(result->cols, 4);
    CHECK_EQ(result->rows, 4);
    CHECK_EQ(result->sprites_per_sheet(), 16);
  }

  TEST_CASE("single sprite") {
    auto result = compute_layout(100, 100, 64, 64);
    CHECK(result);
    CHECK_EQ(result->cols, 1);
    CHECK_EQ(result->rows, 1);
    CHECK_EQ(result->sprites_per_sheet(), 1);
  }

  TEST_CASE("frame larger than max atlas") {
    auto result = compute_layout(32, 32, 64, 64);
    CHECK(result);
    CHECK_EQ(result->cols, 1); // max(1, 32/64) = 1
    CHECK_EQ(result->rows, 1);
    CHECK_EQ(result->sprites_per_sheet(), 1);
  }

  TEST_CASE("wide atlas with many columns") {
    auto result = compute_layout(1024, 64, 32, 32);
    CHECK(result);
    CHECK_EQ(result->cols, 32);
    CHECK_EQ(result->rows, 2);
    CHECK_EQ(result->sprites_per_sheet(), 64);
  }

  TEST_CASE("tall atlas with many rows") {
    auto result = compute_layout(64, 1024, 32, 32);
    CHECK(result);
    CHECK_EQ(result->cols, 2);
    CHECK_EQ(result->rows, 32);
    CHECK_EQ(result->sprites_per_sheet(), 64);
  }

  TEST_CASE("small frame size in large atlas") {
    auto result = compute_layout(2048, 2048, 16, 16);
    CHECK(result);
    CHECK_EQ(result->cols, 128);
    CHECK_EQ(result->rows, 128);
    CHECK_EQ(result->sprites_per_sheet(), 16384);
  }

  TEST_CASE("zero frame width is invalid") {
    auto result = compute_layout(2048, 2048, 0, 64);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid frame size") != std::string::npos);
  }

  TEST_CASE("zero frame height is invalid") {
    auto result = compute_layout(2048, 2048, 64, 0);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid frame size") != std::string::npos);
  }

  TEST_CASE("negative frame width is invalid") {
    auto result = compute_layout(2048, 2048, -1, 64);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid frame size") != std::string::npos);
  }

  TEST_CASE("negative frame height is invalid") {
    auto result = compute_layout(2048, 2048, 64, -1);
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid frame size") != std::string::npos);
  }
}

namespace {
  Sprite make_sprite(int w, int h) {
    Sprite s;
    s.width = w;
    s.height = h;
    s.data.assign(static_cast<std::size_t>(w * h * 4), 0);
    return s;
  }

  void set_pixel(Sprite &s, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const auto idx = static_cast<std::size_t>((y * s.width + x) * 4);
    s.data[idx] = r;
    s.data[idx + 1] = g;
    s.data[idx + 2] = b;
    s.data[idx + 3] = a;
  }

  PackData make_base_data() {
    PackData data;
    data.output_name = "test_output";
    data.layout = {4, 4};
    data.num_sheets = 1;
    data.frame_width = 128;
    data.frame_height = 96;
    return data;
  }

  void setup_output_dir(std::filesystem::path &dir, const char *name) {
    dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::create_directories(dir);
  }

  void teardown_output_dir(const std::filesystem::path &dir) {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
} // namespace

TEST_SUITE("find_visible_bounds") {
  TEST_CASE("fully opaque returns full bounds") {
    auto s = make_sprite(8, 8);
    for (int y = 0; y < 8; ++y)
      for (int x = 0; x < 8; ++x)
        set_pixel(s, x, y, 255, 255, 255, 255);
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.x, 0);
    CHECK_EQ(crop.y, 0);
    CHECK_EQ(crop.w, 8);
    CHECK_EQ(crop.h, 8);
  }

  TEST_CASE("transparent border returns inner bounds") {
    auto s = make_sprite(64, 64);
    for (int y = 16; y < 48; ++y)
      for (int x = 16; x < 48; ++x)
        set_pixel(s, x, y, 128, 128, 128, 128);
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.x, 16);
    CHECK_EQ(crop.y, 16);
    CHECK_EQ(crop.w, 32);
    CHECK_EQ(crop.h, 32);
  }

  TEST_CASE("fully transparent returns empty") {
    auto s = make_sprite(10, 10);
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.w, 0);
    CHECK_EQ(crop.h, 0);
  }

  TEST_CASE("single visible pixel returns 1x1") {
    auto s = make_sprite(16, 16);
    set_pixel(s, 7, 5, 255, 0, 0, 1);
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.x, 7);
    CHECK_EQ(crop.y, 5);
    CHECK_EQ(crop.w, 1);
    CHECK_EQ(crop.h, 1);
  }

  TEST_CASE("invalid sprite returns empty") {
    Sprite s;
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.w, 0);
    CHECK_EQ(crop.h, 0);
  }

  TEST_CASE("alpha zero excluded, alpha one included") {
    auto s = make_sprite(8, 8);
    set_pixel(s, 2, 2, 255, 0, 0, 0);
    set_pixel(s, 5, 5, 0, 255, 0, 1);
    const auto crop = find_visible_bounds(s);
    CHECK_EQ(crop.x, 5);
    CHECK_EQ(crop.y, 5);
    CHECK_EQ(crop.w, 1);
    CHECK_EQ(crop.h, 1);
  }
}

TEST_SUITE("write_metadata") {
  using json = nlohmann::json;

  TEST_CASE("default mode writes tile_width/tile_height and pivot fields") {
    std::filesystem::path dir;
    setup_output_dir(dir, "spritepacker_test_default");
    auto data = make_base_data();
    data.character = false;
    data.sheets_dir = dir;
    const auto result = data.write_metadata();
    CHECK(result.has_value());

    std::ifstream f(dir / "test_output.json");
    REQUIRE(f);
    const auto j = json::parse(f);
    CHECK_EQ(j["tile_width"], 128);
    CHECK_EQ(j["tile_height"], 96);
    CHECK_EQ(j["pivot_x"], 0.5f);
    CHECK_EQ(j["pivot_y"], 0.0f);
    CHECK_EQ(j["columns"], 4);
    CHECK_EQ(j["rows"], 4);
    CHECK(j.contains("path"));
    CHECK_FALSE(j.contains("frame_width"));
    CHECK_FALSE(j.contains("frame_height"));
    CHECK_FALSE(j.contains("offset_x"));
    CHECK_FALSE(j.contains("spacing_x"));

    teardown_output_dir(dir);
  }

  TEST_CASE("character mode writes frame_width/frame_height and offset/spacing") {
    std::filesystem::path dir;
    setup_output_dir(dir, "spritepacker_test_char");
    auto data = make_base_data();
    data.character = true;
    data.sheets_dir = dir;
    const auto result = data.write_metadata();
    CHECK(result.has_value());

    std::ifstream f(dir / "test_output.json");
    REQUIRE(f);
    const auto j = json::parse(f);
    CHECK_EQ(j["frame_width"], 128);
    CHECK_EQ(j["frame_height"], 96);
    CHECK_EQ(j["offset_x"], 0);
    CHECK_EQ(j["offset_y"], 0);
    CHECK_EQ(j["spacing_x"], 0);
    CHECK_EQ(j["spacing_y"], 0);
    CHECK_EQ(j["columns"], 4);
    CHECK_EQ(j["rows"], 4);
    CHECK(j.contains("path"));
    CHECK_FALSE(j.contains("tile_width"));
    CHECK_FALSE(j.contains("tile_height"));
    CHECK_FALSE(j.contains("pivot_x"));
    CHECK_FALSE(j.contains("pivot_y"));

    teardown_output_dir(dir);
  }
}
