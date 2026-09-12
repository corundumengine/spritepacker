// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "utils.hpp"
#include <doctest/doctest.h>

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

TEST_SUITE("parse_size") {
  TEST_CASE("identical width and height") {
    const auto result = parse_size("64x64");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 64);
    CHECK_EQ(h, 64);
  }

  TEST_CASE("different sizes") {
    const auto result = parse_size("128x32");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 128);
    CHECK_EQ(h, 32);
  }

  TEST_CASE("large numbers") {
    const auto result = parse_size("4096x2048");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 4096);
    CHECK_EQ(h, 2048);
  }

  TEST_CASE("missing delimiter is invalid") {
    const auto result = parse_size("64");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("non-numeric is invalid") {
    const auto result = parse_size("axb");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("zero width is invalid") {
    const auto result = parse_size("0x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("zero height is invalid") {
    const auto result = parse_size("64x0");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("negative width is invalid") {
    const auto result = parse_size("-1x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("number out of range is invalid") {
    const auto result = parse_size("999999999999x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("out of range") != std::string::npos);
  }

  TEST_CASE("trailing characters after height is invalid") {
    const auto result = parse_size("64x32trailing");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("garbage between width and delimiter is invalid") {
    const auto result = parse_size("64abcx32");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("missing width is invalid") {
    const auto result = parse_size("x32");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }
}

TEST_SUITE("parse_int") {
  TEST_CASE("parses a positive integer") {
    const auto result = parse_int("2", "--padding");
    CHECK(result);
    CHECK_EQ(*result, 2);
  }

  TEST_CASE("parses zero") {
    const auto result = parse_int("0", "--padding");
    CHECK(result);
    CHECK_EQ(*result, 0);
  }

  TEST_CASE("rejects a negative value") {
    const auto result = parse_int("-1", "--padding");
    CHECK_FALSE(result);
    CHECK(result.error().find("must not be negative") != std::string::npos);
  }

  TEST_CASE("rejects non-numeric input") {
    const auto result = parse_int("abc", "--padding");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid integer") != std::string::npos);
  }

  TEST_CASE("rejects trailing garbage") {
    const auto result = parse_int("2px", "--padding");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid integer") != std::string::npos);
  }
}

TEST_SUITE("parse_double") {
  TEST_CASE("parses a fractional value") {
    const auto result = parse_double("0.18", "--pivot");
    CHECK(result);
    CHECK_EQ(*result, doctest::Approx(0.18));
  }

  TEST_CASE("rejects non-numeric input") {
    const auto result = parse_double("abc", "--pivot");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid number") != std::string::npos);
  }

  TEST_CASE("rejects trailing garbage") {
    const auto result = parse_double("0.5x", "--pivot");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid number") != std::string::npos);
  }

  TEST_CASE("rejects empty input") {
    const auto result = parse_double("", "--pivot");
    CHECK_FALSE(result);
  }
}

TEST_SUITE("resolve_pivot") {
  TEST_CASE("bottom-center is the default") {
    const auto result = resolve_pivot("bottom-center");
    REQUIRE(result);
    CHECK_EQ(result->x, doctest::Approx(0.5));
    CHECK_EQ(result->y, doctest::Approx(1.0));
    CHECK_FALSE(result->full_canvas);
  }

  TEST_CASE("center preset") {
    const auto result = resolve_pivot("center");
    REQUIRE(result);
    CHECK_EQ(result->x, doctest::Approx(0.5));
    CHECK_EQ(result->y, doctest::Approx(0.5));
    CHECK_FALSE(result->full_canvas);
  }

  TEST_CASE("full-canvas without a value is bottom-center of the full canvas") {
    const auto result = resolve_pivot("full-canvas");
    REQUIRE(result);
    CHECK_EQ(result->x, doctest::Approx(0.5));
    CHECK_EQ(result->y, doctest::Approx(0.0));
    CHECK(result->full_canvas);
  }

  TEST_CASE("full-canvas single value sets y") {
    const auto result = resolve_pivot("full-canvas:0.18");
    REQUIRE(result);
    CHECK_EQ(result->x, doctest::Approx(0.5));
    CHECK_EQ(result->y, doctest::Approx(0.18));
    CHECK(result->full_canvas);
  }

  TEST_CASE("full-canvas x,y value sets both") {
    const auto result = resolve_pivot("full-canvas:0.5,0.18");
    REQUIRE(result);
    CHECK_EQ(result->x, doctest::Approx(0.5));
    CHECK_EQ(result->y, doctest::Approx(0.18));
    CHECK(result->full_canvas);
  }

  TEST_CASE("unknown preset is rejected") {
    const auto result = resolve_pivot("bogus");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid --pivot value") != std::string::npos);
  }

  TEST_CASE("non-numeric full-canvas suffix is rejected") {
    for (const std::string_view preset :
         {"full-canvas:", "full-canvas:abc", "full-canvas:0.5,abc", "full-canvas:,0.18"}) {
      CAPTURE(preset);
      CHECK_FALSE(resolve_pivot(preset));
    }
  }
}

TEST_SUITE("validate_animation_naming") {
  TEST_CASE("passes when all facings of an animation share the same frame set") {
    const std::vector<std::string> files{
        "knight_walk_north_0.png",
        "knight_walk_north_1.png",
        "knight_walk_south_0.png",
        "knight_walk_south_1.png",
    };
    CHECK(validate_animation_naming(files));
  }

  TEST_CASE("fails when a facing is missing a frame present in another facing") {
    const std::vector<std::string> files{
        "knight_walk_north_0.png",
        "knight_walk_north_1.png",
        "knight_walk_south_0.png", // missing frame 1
    };
    const auto result = validate_animation_naming(files);
    CHECK_FALSE(result);
    CHECK(result.error().find("south") != std::string::npos);
    CHECK(result.error().find('1') != std::string::npos);
  }

  TEST_CASE("ignores files that don't match the naming convention") {
    const std::vector<std::string> files{"terrain.png", "chest_open.png", "background.png"};
    CHECK(validate_animation_naming(files));
  }

  TEST_CASE("a single facing with no siblings is not flagged") {
    const std::vector<std::string> files{"knight_walk_north_0.png", "knight_walk_north_1.png"};
    CHECK(validate_animation_naming(files));
  }
}

TEST_SUITE("glob_to_regex") {
  TEST_CASE("star wildcard matches multiple files") {
    const auto re = glob_to_regex("*.png");
    CHECK(std::regex_match("sprite.png", re));
    CHECK(std::regex_match("SPRITE.PNG", re)); // icase
    CHECK_FALSE(std::regex_match("sprite.jpg", re));
  }

  TEST_CASE("question mark matches single character") {
    const auto re = glob_to_regex("tile?.png");
    CHECK(std::regex_match("tile1.png", re));
    CHECK(std::regex_match("tileA.png", re));
    CHECK_FALSE(std::regex_match("tile12.png", re));
    CHECK_FALSE(std::regex_match("tile.png", re));
  }

  TEST_CASE("dot is escaped, not treated as any character") {
    const auto re = glob_to_regex("a.b");
    CHECK_FALSE(std::regex_match("acb", re));
    CHECK(std::regex_match("a.b", re));
  }

  TEST_CASE("case insensitive matching") {
    const auto re = glob_to_regex("*.PNG");
    CHECK(std::regex_match("sprite.png", re));
    CHECK(std::regex_match("SPRITE.PNG", re));
  }

  TEST_CASE("partial prefix wildcard") {
    const auto re = glob_to_regex("chest_*");
    CHECK(std::regex_match("chest_open.png", re));
    CHECK(std::regex_match("chest_closed.png", re));
    CHECK_FALSE(std::regex_match("open.png", re));
  }

  TEST_CASE("multiple wildcards") {
    const auto re = glob_to_regex("*.*.png");
    CHECK(std::regex_match("sprite.1.png", re));
    CHECK(std::regex_match("a.b.png", re));
  }

  TEST_CASE("only question marks") {
    const auto re = glob_to_regex("???");
    CHECK(std::regex_match("abc", re));
    CHECK_FALSE(std::regex_match("ab", re));
    CHECK_FALSE(std::regex_match("abcd", re));
  }

  TEST_CASE("exact match with no wildcards") {
    const auto re = glob_to_regex("sprite.png");
    CHECK(std::regex_match("sprite.png", re));
    CHECK_FALSE(std::regex_match("sprite2.png", re));
  }

  TEST_CASE("square brackets are preserved") {
    const auto re = glob_to_regex("[abc]");
    CHECK(std::regex_match("a", re));
    CHECK(std::regex_match("b", re));
    CHECK_FALSE(std::regex_match("d", re));
  }

  TEST_CASE("regex metacharacters in filename are escaped") {
    const auto re = glob_to_regex("file+(1).png");
    CHECK(std::regex_match("file+(1).png", re));
    CHECK_FALSE(std::regex_match("fileXXX1Xpng", re));
  }

  TEST_CASE("bracket negation [!...] excludes listed characters") {
    const auto re = glob_to_regex("[!abc].png");
    CHECK(std::regex_match("d.png", re));
    CHECK_FALSE(std::regex_match("a.png", re));
    CHECK_FALSE(std::regex_match("b.png", re));
  }

  TEST_CASE("exclamation outside brackets is a literal character") {
    const auto re = glob_to_regex("file!.png");
    CHECK(std::regex_match("file!.png", re));
    CHECK_FALSE(std::regex_match("file.png", re));
  }

  TEST_CASE("unclosed bracket with content is auto-closed") {
    const auto re = glob_to_regex("tile[abc");
    CHECK(std::regex_match("tilea", re));
    CHECK(std::regex_match("tileb", re));
    CHECK_FALSE(std::regex_match("tiled", re));
    CHECK_FALSE(std::regex_match("tile[abc", re));
  }

  TEST_CASE("unclosed empty bracket matches literal '['") {
    const auto re = glob_to_regex("file[");
    CHECK(std::regex_match("file[", re));
    CHECK_FALSE(std::regex_match("file", re));
    CHECK_FALSE(std::regex_match("filea", re));
  }

  TEST_CASE("unclosed negated bracket is auto-closed") {
    const auto re = glob_to_regex("tile[!xyz");
    CHECK(std::regex_match("tileA", re));
    CHECK_FALSE(std::regex_match("tilex", re));
    CHECK_FALSE(std::regex_match("tiley", re));
  }
}

TEST_SUITE("resolve_input_files") {
  TEST_CASE("only PNG files are allowed") {
    const auto result = resolve_input_files(fs::current_path(), "image.jpg");
    CHECK_FALSE(result);
  }

  TEST_CASE("directory with no pngs returns an error") {
    const auto result = resolve_input_files(fs::current_path(), "");
    CHECK_FALSE(result);
    CHECK(result.error().find("No PNG files found") != std::string::npos);
  }

  TEST_CASE("non-existent directory returns an error") {
    const auto result = resolve_input_files("/spritepacker_nonexistent_dir_abc123", "");
    CHECK_FALSE(result);
    CHECK(result.error().find("Cannot read directory") != std::string::npos);
  }

  TEST_CASE("uppercase glob pattern finds lowercase .png files") {
    const auto result = resolve_input_files(fs::path{TEST_FIXTURES_DIR}, "*.PNG");
    CHECK(result);
    if (result)
      CHECK_EQ(result->size(), 1u);
  }
}
