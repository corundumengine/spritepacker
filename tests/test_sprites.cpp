#include "sprites.hpp"
#include <doctest/doctest.h>

TEST_SUITE("Sprite::is_valid") {
  TEST_CASE("valid image is not empty") {
    Sprite img;
    img.data.push_back(255);
    img.width = 1;
    img.height = 1;
    CHECK(img.is_valid());
  }

  TEST_CASE("large image is valid") {
    Sprite img;
    img.data.resize(1024 * 1024 * 4);
    img.width = 1024;
    img.height = 1024;
    CHECK(img.is_valid());
  }

  TEST_CASE("blank image is invalid") {
    Sprite img;
    img.width = 1;
    img.height = 1;
    CHECK_FALSE(img.is_valid());
  }

  TEST_CASE("zero width is invalid") {
    Sprite img;
    img.data.push_back(255);
    img.width = 0;
    img.height = 1;
    CHECK_FALSE(img.is_valid());
  }

  TEST_CASE("zero height is invalid") {
    Sprite img;
    img.data.push_back(255);
    img.width = 1;
    img.height = 0;
    CHECK_FALSE(img.is_valid());
  }

  TEST_CASE("negative width is invalid") {
    Sprite img;
    img.data.push_back(255);
    img.width = -1;
    img.height = 1;
    CHECK_FALSE(img.is_valid());
  }

  TEST_CASE("all zeros is invalid") {
    Sprite img;
    CHECK_FALSE(img.is_valid());
  }
}

TEST_SUITE("Sprite::load") {
  TEST_CASE("loads a valid PNG") {
    Sprite img;
    auto result = img.load(TEST_FIXTURES_DIR "/sprite.png");
    CHECK(result);
    CHECK(img.is_valid());
  }

  TEST_CASE("returns an error for a non-existent file") {
    Sprite img;
    auto result = img.load(TEST_FIXTURES_DIR "/nonexistent.png");
    CHECK_FALSE(result);
  }
}
