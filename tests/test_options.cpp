#include "options.hpp"
#include <doctest/doctest.h>

TEST_SUITE("Options::parse_args") {
  TEST_CASE("parses valid options") {
    const char *argv[] = {"prog",    "--input",    "/src",   "--output", "/out",       "--name",   "atlas",
                          "--files", "chest*.png", "--size", "32x32",    "--max-size", "1024x1024"};
    auto result = Options::parse_args(13, const_cast<char **>(argv));
    CHECK(result);
    CHECK_EQ(result->input, "/src");
    CHECK_EQ(result->output, "/out");
    CHECK_EQ(result->name, "atlas");
    CHECK_EQ(result->files, "chest*.png");
    CHECK_EQ(result->size, "32x32");
    CHECK_EQ(result->max_size, "1024x1024");
  }

  TEST_CASE("returns an error with invalid arguments") {
    const char *argv[] = {"prog", "--input", ".", "--output", ".", "--name", "test", "--foo", "bar"};
    auto result = Options::parse_args(std::size(argv), const_cast<char **>(argv));
    CHECK_FALSE(result);
    CHECK(result.error().find("Unknown argument '--foo'") != std::string::npos);
  }
}

TEST_SUITE("Options::validate") {
  TEST_CASE("valid options") {
    Options opts;
    opts.input = "/src";
    opts.output = "/out";
    opts.name = "atlas";
    CHECK(opts.validate());
  }

  TEST_CASE("source is required") {
    Options opts;
    opts.output = "/out";
    opts.name = "atlas";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --input") != std::string::npos);
  }

  TEST_CASE("output is required") {
    Options opts;
    opts.input = "/src";
    opts.name = "atlas";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --output") != std::string::npos);
  }

  TEST_CASE("name is required") {
    Options opts;
    opts.input = "/src";
    opts.output = "/out";
    auto result = opts.validate();
    CHECK_FALSE(result);
    CHECK(result.error().find("Missing required argument: --name") != std::string::npos);
  }
}
