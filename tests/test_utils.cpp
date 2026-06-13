#include "utils.hpp"
#include <doctest/doctest.h>

#include <filesystem>

namespace fs = std::filesystem;

TEST_SUITE("parse_size") {
  TEST_CASE("identical width and height") {
    auto result = parse_size("64x64");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 64);
    CHECK_EQ(h, 64);
  }

  TEST_CASE("different sizes") {
    auto result = parse_size("128x32");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 128);
    CHECK_EQ(h, 32);
  }

  TEST_CASE("large numbers") {
    auto result = parse_size("4096x2048");
    CHECK(result);
    auto [w, h] = *result;
    CHECK_EQ(w, 4096);
    CHECK_EQ(h, 2048);
  }

  TEST_CASE("missing delimiter is invalid") {
    auto result = parse_size("64");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("non-numeric is invalid") {
    auto result = parse_size("axb");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("zero width is invalid") {
    auto result = parse_size("0x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("zero height is invalid") {
    auto result = parse_size("64x0");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("negative width is invalid") {
    auto result = parse_size("-1x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("must be positive") != std::string::npos);
  }

  TEST_CASE("number out of range is invalid") {
    auto result = parse_size("999999999999x64");
    CHECK_FALSE(result);
    CHECK(result.error().find("out of range") != std::string::npos);
  }

  TEST_CASE("trailing characters after height is invalid") {
    auto result = parse_size("64x32trailing");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("garbage between width and delimiter is invalid") {
    auto result = parse_size("64abcx32");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }

  TEST_CASE("missing width is invalid") {
    auto result = parse_size("x32");
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid size format") != std::string::npos);
  }
}

TEST_SUITE("glob_to_regex") {
  TEST_CASE("star wildcard matches multiple files") {
    auto re = glob_to_regex("*.png");
    CHECK(std::regex_match("sprite.png", re));
    CHECK(std::regex_match("SPRITE.PNG", re)); // icase
    CHECK_FALSE(std::regex_match("sprite.jpg", re));
  }

  TEST_CASE("question mark matches single character") {
    auto re = glob_to_regex("tile?.png");
    CHECK(std::regex_match("tile1.png", re));
    CHECK(std::regex_match("tileA.png", re));
    CHECK_FALSE(std::regex_match("tile12.png", re));
    CHECK_FALSE(std::regex_match("tile.png", re));
  }

  TEST_CASE("dot is escaped, not treated as any character") {
    auto re = glob_to_regex("a.b");
    CHECK_FALSE(std::regex_match("acb", re));
    CHECK(std::regex_match("a.b", re));
  }

  TEST_CASE("case insensitive matching") {
    auto re = glob_to_regex("*.PNG");
    CHECK(std::regex_match("sprite.png", re));
    CHECK(std::regex_match("SPRITE.PNG", re));
  }

  TEST_CASE("partial prefix wildcard") {
    auto re = glob_to_regex("chest_*");
    CHECK(std::regex_match("chest_open.png", re));
    CHECK(std::regex_match("chest_closed.png", re));
    CHECK_FALSE(std::regex_match("open.png", re));
  }

  TEST_CASE("multiple wildcards") {
    auto re = glob_to_regex("*.*.png");
    CHECK(std::regex_match("sprite.1.png", re));
    CHECK(std::regex_match("a.b.png", re));
  }

  TEST_CASE("only question marks") {
    auto re = glob_to_regex("???");
    CHECK(std::regex_match("abc", re));
    CHECK_FALSE(std::regex_match("ab", re));
    CHECK_FALSE(std::regex_match("abcd", re));
  }

  TEST_CASE("exact match with no wildcards") {
    auto re = glob_to_regex("sprite.png");
    CHECK(std::regex_match("sprite.png", re));
    CHECK_FALSE(std::regex_match("sprite2.png", re));
  }

  TEST_CASE("square brackets are preserved") {
    auto re = glob_to_regex("[abc]");
    CHECK(std::regex_match("a", re));
    CHECK(std::regex_match("b", re));
    CHECK_FALSE(std::regex_match("d", re));
  }

  TEST_CASE("regex metacharacters in filename are escaped") {
    auto re = glob_to_regex("file+(1).png");
    CHECK(std::regex_match("file+(1).png", re));
    CHECK_FALSE(std::regex_match("fileXXX1Xpng", re));
  }

  TEST_CASE("bracket negation [!...] excludes listed characters") {
    auto re = glob_to_regex("[!abc].png");
    CHECK(std::regex_match("d.png", re));
    CHECK_FALSE(std::regex_match("a.png", re));
    CHECK_FALSE(std::regex_match("b.png", re));
  }

  TEST_CASE("exclamation outside brackets is a literal character") {
    auto re = glob_to_regex("file!.png");
    CHECK(std::regex_match("file!.png", re));
    CHECK_FALSE(std::regex_match("file.png", re));
  }

  TEST_CASE("unclosed bracket with content is auto-closed") {
    auto re = glob_to_regex("tile[abc");
    CHECK(std::regex_match("tilea", re));
    CHECK(std::regex_match("tileb", re));
    CHECK_FALSE(std::regex_match("tiled", re));
    CHECK_FALSE(std::regex_match("tile[abc", re));
  }

  TEST_CASE("unclosed empty bracket matches literal '['") {
    auto re = glob_to_regex("file[");
    CHECK(std::regex_match("file[", re));
    CHECK_FALSE(std::regex_match("file", re));
    CHECK_FALSE(std::regex_match("filea", re));
  }

  TEST_CASE("unclosed negated bracket is auto-closed") {
    auto re = glob_to_regex("tile[!xyz");
    CHECK(std::regex_match("tileA", re));
    CHECK_FALSE(std::regex_match("tilex", re));
    CHECK_FALSE(std::regex_match("tiley", re));
  }
}

TEST_SUITE("resolve_input_files") {
  TEST_CASE("only PNG files are allowed") {
    auto result = resolve_input_files(fs::current_path(), "image.jpg");
    CHECK_FALSE(result);
  }

  TEST_CASE("directory with no pngs returns an error") {
    auto result = resolve_input_files(fs::current_path(), "");
    CHECK_FALSE(result);
    CHECK(result.error().find("No PNG files found") != std::string::npos);
  }

  TEST_CASE("non-existent directory returns an error") {
    auto result = resolve_input_files("/spritepacker_nonexistent_dir_abc123", "");
    CHECK_FALSE(result);
    CHECK(result.error().find("Cannot read directory") != std::string::npos);
  }

  TEST_CASE("uppercase glob pattern finds lowercase .png files") {
    auto result = resolve_input_files(fs::path{TEST_FIXTURES_DIR}, "*.PNG");
    CHECK(result);
    if (result)
      CHECK_EQ(result->size(), 1u);
  }
}
