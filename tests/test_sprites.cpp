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

namespace {

  // Builds a w x h RGBA sprite, fully transparent except for opaque pixels within
  // [ox, ox+ow) x [oy, oy+oh).
  Sprite make_sprite(int w, int h, int ox, int oy, int ow, int oh) {
    Sprite s;
    s.width = w;
    s.height = h;
    s.data.assign(static_cast<std::size_t>(w * h * 4), 0);
    for (int y = oy; y < oy + oh; ++y) {
      for (int x = ox; x < ox + ow; ++x) {
        const auto idx = static_cast<std::size_t>((y * w + x) * 4);
        s.data[idx + 0] = 255;
        s.data[idx + 1] = 255;
        s.data[idx + 2] = 255;
        s.data[idx + 3] = 255; // opaque
      }
    }
    return s;
  }

} // namespace

TEST_SUITE("compute_trim") {
  TEST_CASE("fully opaque sprite trims to the whole frame") {
    const Sprite s = make_sprite(64, 32, 0, 0, 64, 32);
    const TrimRect t = compute_trim(s);
    CHECK_EQ(t.x, 0);
    CHECK_EQ(t.y, 0);
    CHECK_EQ(t.w, 64);
    CHECK_EQ(t.h, 32);
  }

  TEST_CASE("centered content trims to its exact bounding box") {
    const Sprite s = make_sprite(256, 256, 62, 93, 132, 71);
    const TrimRect t = compute_trim(s);
    CHECK_EQ(t.x, 62);
    CHECK_EQ(t.y, 93);
    CHECK_EQ(t.w, 132);
    CHECK_EQ(t.h, 71);
  }

  TEST_CASE("content touching the top-left corner trims to x=0, y=0") {
    const Sprite s = make_sprite(100, 100, 0, 0, 10, 20);
    const TrimRect t = compute_trim(s);
    CHECK_EQ(t.x, 0);
    CHECK_EQ(t.y, 0);
    CHECK_EQ(t.w, 10);
    CHECK_EQ(t.h, 20);
  }

  TEST_CASE("fully transparent sprite falls back to the whole frame") {
    Sprite s;
    s.width = 40;
    s.height = 20;
    s.data.assign(static_cast<std::size_t>(40 * 20 * 4), 0);
    const TrimRect t = compute_trim(s);
    CHECK_EQ(t.x, 0);
    CHECK_EQ(t.y, 0);
    CHECK_EQ(t.w, 40);
    CHECK_EQ(t.h, 20);
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
