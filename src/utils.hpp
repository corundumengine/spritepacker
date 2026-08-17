#ifndef UTILS_HPP
#define UTILS_HPP

#include <expected>
#include <filesystem>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

/**
 * Converts a glob pattern to a case-insensitive std::regex. Supports *, ?,
 * [abc], and [!abc]. Other regex metacharacters are escaped to match literally.
 */
[[nodiscard]] std::regex glob_to_regex(std::string_view pattern);

/** Parses a size string in "WxH" format (e.g. "64x64") into a (width, height) pair. */
[[nodiscard]] std::expected<std::tuple<int, int>, std::string> parse_size(std::string_view size_str);

/** Parses a non-negative integer option value (e.g. for --padding). */
[[nodiscard]] std::expected<int, std::string> parse_int(std::string_view value, std::string_view option_name);

/** Parses a floating-point option value (e.g. a pivot coordinate). */
[[nodiscard]] std::expected<double, std::string> parse_double(std::string_view value, std::string_view option_name);

/**
 * A resolved sprite pivot: normalized (0..1) coordinates plus the basis they're measured in.
 * When @p full_canvas is false, @p x/@p y are fractions of the trimmed sprite box (y=0 top,
 * y=1 bottom). When true, they're fractions of the full source canvas (y measured from the
 * bottom), preserving authored padding.
 */
struct Pivot {
  double x{};
  double y{};
  bool full_canvas{false};
};

/**
 * Resolves a --pivot value into normalized coordinates and their basis.
 *
 * Presets anchor the *trimmed* art (y from the top): "bottom-center" -> (0.5, 1.0),
 * "center" -> (0.5, 0.5), "top-center" -> (0.5, 0.0), "top-left" -> (0.0, 0.0).
 * "full-canvas" anchors the full source canvas (y from the bottom) and takes an optional
 * value suffix: "full-canvas" -> (0.5, 0.0), "full-canvas:0.18" -> (0.5, 0.18),
 * "full-canvas:0.5,0.18" -> (0.5, 0.18).
 */
[[nodiscard]] std::expected<Pivot, std::string> resolve_pivot(std::string_view preset);

/**
 * Resolves a comma-separated file list or glob patterns to absolute PNG paths.
 * If files_str is empty, returns all PNGs in source_dir sorted alphabetically.
 * Otherwise each comma-separated token is matched as an exact filename or glob.
 * Duplicates are silently skipped.
 */
[[nodiscard]] std::expected<std::vector<std::string>, std::string>
resolve_input_files(const std::filesystem::path &source_dir, std::string_view files_str);

/**
 * Validates that @p files follow the `<unit>_<state>_<facing>_<frame>.png` naming convention and
 * that every (unit, state) group has the same set of frame indices for every facing it uses —
 * i.e. no facing is missing frames that another facing of the same animation has.
 *
 * Files whose stem does not split into at least 4 underscore-separated tokens with a numeric
 * final token are ignored by this check (they're assumed not to be part of a directional
 * animation set). Returns an error describing every gap found, or success if there are none.
 */
[[nodiscard]] std::expected<void, std::string> validate_animation_naming(const std::vector<std::string> &files);

#endif // UTILS_HPP
