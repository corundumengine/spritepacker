#include "options.hpp"
#include <doctest/doctest.h>

TEST_SUITE("Options::parse_args") {
  TEST_CASE("parses valid options") {
    const char *argv[] = {"prog",       "--input",    "/src",      "--sheets",  "/out", "--name",  "atlas", "--files",
                          "chest*.png", "--max-size", "1024x1024", "--padding", "2",    "--pivot", "center"};
    auto result = Options::parse_args(std::size(argv), const_cast<char **>(argv));
    CHECK(result);
    CHECK_EQ(result->input, "/src");
    CHECK_EQ(result->sheets, "/out");
    CHECK_EQ(result->name, "atlas");
    CHECK_EQ(result->files, "chest*.png");
    CHECK_EQ(result->max_size, "1024x1024");
    CHECK_EQ(result->padding, "2");
    CHECK_EQ(result->pivot, "center");
  }

  TEST_CASE("parses boolean flags") {
    const char *argv[] = {"prog", "--input", "/src", "--sheets", "/out", "--name", "atlas", "--validate-animations",
                          "--pot"};
    auto result = Options::parse_args(std::size(argv), const_cast<char **>(argv));
    CHECK(result);
    CHECK(result->validate_animations);
    CHECK(result->pot);
  }

  TEST_CASE("returns an error with invalid arguments") {
    const char *argv[] = {"prog", "--input", ".", "--sheets", ".", "--name", "test", "--foo", "bar"};
    auto result = Options::parse_args(std::size(argv), const_cast<char **>(argv));
    CHECK_FALSE(result);
    CHECK(result.error().find("Unknown argument '--foo'") != std::string::npos);
  }

  TEST_CASE("rejects flag where value is expected") {
    const char *argv[] = {"prog", "--input", "-h"};
    auto result = Options::parse_args(std::size(argv), const_cast<char **>(argv));
    CHECK_FALSE(result);
    CHECK(result.error().find("Unexpected flag") != std::string::npos);
  }
}

TEST_SUITE("Options::validate") {
  TEST_CASE("valid options") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    opts.name = "atlas";
    CHECK(opts.validate());
  }

  TEST_CASE("source is required") {
    Options opts;
    opts.sheets = "/out";
    opts.name = "atlas";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --input") != std::string::npos);
  }

  TEST_CASE("sheets is required") {
    Options opts;
    opts.input = "/src";
    opts.name = "atlas";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --sheets") != std::string::npos);
  }

  TEST_CASE("name is required") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --name") != std::string::npos);
  }

  TEST_CASE("default pivot preset is valid") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    opts.name = "atlas";
    CHECK(opts.validate());
  }

  TEST_CASE("unknown pivot preset is rejected") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    opts.name = "atlas";
    opts.pivot = "bogus";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Invalid --pivot value") != std::string::npos);
  }

  TEST_CASE("full-canvas pivot presets with value suffixes are valid") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    opts.name = "atlas";
    for (const std::string pivot : {"full-canvas", "full-canvas:0.18", "full-canvas:0.5,0.18"}) {
      opts.pivot = pivot;
      CAPTURE(pivot);
      CHECK(opts.validate());
    }
  }

  TEST_CASE("full-canvas value suffix without a valid number is rejected") {
    Options opts;
    opts.input = "/src";
    opts.sheets = "/out";
    opts.name = "atlas";
    for (const std::string_view pivot : {"full-canvas:", "full-canvas:abc", "full-canvas:0.5,abc", "full-canvas:,0.18"}) {
      opts.pivot = pivot;
      CAPTURE(pivot);
      auto result = opts.validate();
      CHECK_FALSE(result);
    }
  }
}
