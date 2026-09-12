// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "rect_packer.hpp"
#include <doctest/doctest.h>

#include <optional>
#include <utility>
#include <vector>

namespace {
  bool rects_overlap(const PackedRect &a, const PackedRect &b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
  }

  PackedRect require_rect(const std::optional<PackedRect> &rect) {
    REQUIRE(rect.has_value());
    if (!rect) {
      return {};
    }
    return *rect;
  }

  void check_no_overlaps(const PackedRect &rect, const std::vector<PackedRect> &placed) {
    for (const auto &other : placed) {
      CHECK_FALSE(rects_overlap(rect, other));
    }
  }
} // namespace

TEST_SUITE("MaxRectsPacker") {
  TEST_CASE("places a single rect at the origin") {
    MaxRectsPacker packer(256, 256);
    const auto rect = require_rect(packer.insert(64, 32));
    CHECK_EQ(rect.x, 0);
    CHECK_EQ(rect.y, 0);
    CHECK_EQ(rect.w, 64);
    CHECK_EQ(rect.h, 32);
  }

  TEST_CASE("rejects a rect larger than the bin in either dimension") {
    MaxRectsPacker packer(64, 64);
    CHECK_FALSE(packer.insert(65, 32));
    CHECK_FALSE(packer.insert(32, 65));
  }

  TEST_CASE("rejects zero or negative dimensions") {
    MaxRectsPacker packer(64, 64);
    CHECK_FALSE(packer.insert(0, 10));
    CHECK_FALSE(packer.insert(10, 0));
    CHECK_FALSE(packer.insert(-1, 10));
  }

  TEST_CASE("fills a bin exactly with same-size rects and no overlap") {
    MaxRectsPacker packer(64, 64);
    std::vector<PackedRect> placed;
    for (int i = 0; i < 16; ++i) {
      const auto rect = require_rect(packer.insert(16, 16));
      check_no_overlaps(rect, placed);
      placed.push_back(rect);
    }
    // The bin is exactly full now; a 17th same-size rect must fail.
    CHECK_FALSE(packer.insert(16, 16));
  }

  TEST_CASE("packs a mix of odd sizes without overlap") {
    MaxRectsPacker packer(200, 200);
    const std::vector<std::pair<int, int>> sizes{{100, 40}, {30, 30}, {70, 90}, {10, 10}, {50, 50}, {1, 1}};
    std::vector<PackedRect> placed;
    for (const auto &[w, h] : sizes) {
      const auto rect = require_rect(packer.insert(w, h));
      CHECK_EQ(rect.w, w);
      CHECK_EQ(rect.h, h);
      check_no_overlaps(rect, placed);
      placed.push_back(rect);
    }
  }

  TEST_CASE("returns nullopt once the bin has no room left for a given size") {
    MaxRectsPacker packer(10, 10);
    REQUIRE(packer.insert(10, 10));
    CHECK_FALSE(packer.insert(1, 1));
  }

  TEST_CASE("a single sprite exactly filling the bin succeeds") {
    MaxRectsPacker packer(128, 96);
    const auto rect = require_rect(packer.insert(128, 96));
    CHECK_EQ(rect.x, 0);
    CHECK_EQ(rect.y, 0);
  }
}
