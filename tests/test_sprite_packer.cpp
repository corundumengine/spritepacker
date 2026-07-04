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
    CHECK_EQ(result->cols, 1);
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

TEST_SUITE("write_metadata") {
  using json = nlohmann::json;

  TEST_CASE("writes frame_width/frame_height and offset/spacing fields") {
    PackData data;
    data.output_name = "test_output";
    data.layout = {4, 4};
    data.num_sheets = 1;
    data.frame_width = 128;
    data.frame_height = 96;

    std::filesystem::path dir = std::filesystem::temp_directory_path() / "spritepacker_test_meta";
    std::filesystem::create_directories(dir);
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

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
}
