// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "options.hpp"
#include <doctest/doctest.h>

#include <expected>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace {

  std::expected<Options, std::string> run_parse_args(std::initializer_list<std::string_view> args) {
    std::vector<std::string> storage(args.begin(), args.end());
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (auto &arg : storage) {
      argv.push_back(arg.data());
    }
    return Options::parse_args(argv);
  }

} // namespace

TEST_SUITE("Options::parse_args") {
  TEST_CASE("parses valid options") {
    auto result = run_parse_args({
        "prog",
        "--input",
        "/src",
        "--sheets",
        "/out",
        "--name",
        "atlas",
        "--files",
        "chest*.png",
        "--max-size",
        "1024x1024",
        "--padding",
        "2",
        "--pivot",
        "center",
    });
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
    auto result = run_parse_args({
        "prog",
        "--input",
        "/src",
        "--sheets",
        "/out",
        "--name",
        "atlas",
        "--validate-animations",
        "--pot",
    });
    CHECK(result);
    CHECK(result->validate_animations);
    CHECK(result->pot);
  }

  TEST_CASE("returns an error with invalid arguments") {
    auto result = run_parse_args({"prog", "--input", ".", "--sheets", ".", "--name", "test", "--foo", "bar"});
    CHECK_FALSE(result);
    CHECK(result.error().find("Unknown argument '--foo'") != std::string::npos);
  }

  TEST_CASE("rejects flag where value is expected") {
    auto result = run_parse_args({"prog", "--input", "-h"});
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
    for (const std::string_view pivot :
         {"full-canvas:", "full-canvas:abc", "full-canvas:0.5,abc", "full-canvas:,0.18"}) {
      opts.pivot = pivot;
      CAPTURE(pivot);
      const auto result = opts.validate();
      CHECK_FALSE(result);
    }
  }
}
