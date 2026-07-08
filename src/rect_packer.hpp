#ifndef RECT_PACKER_HPP
#define RECT_PACKER_HPP

#include <optional>
#include <vector>

/** A placed (or free) axis-aligned rectangle within a bin, in bin-local pixel coordinates. */
struct PackedRect {
  int x{}, y{}, w{}, h{};
};

/**
 * Places axis-aligned rectangles into a fixed-size bin without overlap, using the MaxRects
 * algorithm (best-short-side-fit placement heuristic, no rotation). Unlike a fixed grid, each
 * rectangle only consumes the space it actually needs, so a bin can hold a mix of very different
 * sprite sizes efficiently — the common case for trimmed isometric sprites.
 */
class MaxRectsPacker {
public:
  MaxRectsPacker(int width, int height);

  /**
   * Attempts to place a w x h rectangle into the bin. Returns its bin-local position and size on
   * success, or std::nullopt if it does not fit in any remaining free space (the caller should
   * start a new bin/sheet in that case).
   */
  [[nodiscard]] std::optional<PackedRect> insert(int w, int h);

private:
  std::vector<PackedRect> free_rects_;
  int width_;
  int height_;

  [[nodiscard]] std::optional<PackedRect> find_position(int w, int h) const;
  void split_free_rect(const PackedRect &free_rect, const PackedRect &placed);
  void prune_free_rects();
};

#endif // RECT_PACKER_HPP
