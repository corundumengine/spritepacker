// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "rect_packer.hpp"

#include <algorithm>
#include <limits>

MaxRectsPacker::MaxRectsPacker(int width, int height) : width_(width), height_(height) {
  free_rects_.push_back(PackedRect{0, 0, width, height});
}

namespace {
  bool contains(const PackedRect &a, const PackedRect &b) {
    return b.x >= a.x && b.y >= a.y && b.x + b.w <= a.x + a.w && b.y + b.h <= a.y + a.h;
  }

  bool overlaps(const PackedRect &a, const PackedRect &b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
  }
} // namespace

std::optional<PackedRect> MaxRectsPacker::find_position(int w, int h) const {
  // Best-short-side-fit: among free rects that can hold w x h, pick the one that leaves the
  // smallest leftover on its shorter side. This tends to keep large free areas intact for later,
  // larger sprites rather than fragmenting the bin greedily.
  std::optional<PackedRect> best;
  int best_short_side_fit{std::numeric_limits<int>::max()};
  int best_long_side_fit{std::numeric_limits<int>::max()};

  for (const auto &free_rect : free_rects_) {
    if (free_rect.w < w || free_rect.h < h)
      continue;
    const int leftover_w{free_rect.w - w};
    const int leftover_h{free_rect.h - h};
    const int short_side{std::min(leftover_w, leftover_h)};
    const int long_side{std::max(leftover_w, leftover_h)};
    if (short_side < best_short_side_fit || (short_side == best_short_side_fit && long_side < best_long_side_fit)) {
      best = PackedRect{free_rect.x, free_rect.y, w, h};
      best_short_side_fit = short_side;
      best_long_side_fit = long_side;
    }
  }
  return best;
}

void MaxRectsPacker::split_free_rect(const PackedRect &free_rect, const PackedRect &placed) {
  if (!overlaps(free_rect, placed))
    return;

  if (placed.x > free_rect.x)
    free_rects_.push_back(PackedRect{free_rect.x, free_rect.y, placed.x - free_rect.x, free_rect.h});
  if (placed.x + placed.w < free_rect.x + free_rect.w)
    free_rects_.push_back(
        PackedRect{placed.x + placed.w, free_rect.y, free_rect.x + free_rect.w - (placed.x + placed.w), free_rect.h});
  if (placed.y > free_rect.y)
    free_rects_.push_back(PackedRect{free_rect.x, free_rect.y, free_rect.w, placed.y - free_rect.y});
  if (placed.y + placed.h < free_rect.y + free_rect.h)
    free_rects_.push_back(
        PackedRect{free_rect.x, placed.y + placed.h, free_rect.w, free_rect.y + free_rect.h - (placed.y + placed.h)});
}

void MaxRectsPacker::prune_free_rects() {
  // Drop any free rect that is fully contained within another — it can't hold anything that the
  // containing rect couldn't already hold, and keeping it around only slows future searches.
  for (std::size_t i = 0; i < free_rects_.size();) {
    bool removed{false};
    for (std::size_t j = 0; j < free_rects_.size(); ++j) {
      if (i == j)
        continue;
      if (contains(free_rects_[j], free_rects_[i])) {
        free_rects_.erase(free_rects_.begin() + static_cast<std::ptrdiff_t>(i));
        removed = true;
        break;
      }
    }
    if (!removed)
      ++i;
  }
}

std::optional<PackedRect> MaxRectsPacker::insert(int w, int h) {
  if (w <= 0 || h <= 0 || w > width_ || h > height_)
    return std::nullopt;

  const auto placed = find_position(w, h);
  if (!placed)
    return std::nullopt;

  const std::vector<PackedRect> old_free_rects{std::move(free_rects_)};
  free_rects_.clear();
  for (const auto &free_rect : old_free_rects) {
    if (overlaps(free_rect, *placed))
      split_free_rect(free_rect, *placed);
    else
      free_rects_.push_back(free_rect);
  }
  prune_free_rects();

  return placed;
}
