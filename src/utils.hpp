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

/**
 * Resolves a comma-separated file list or glob patterns to absolute PNG paths.
 * If files_str is empty, returns all PNGs in source_dir sorted alphabetically.
 * Otherwise each comma-separated token is matched as an exact filename or glob.
 * Duplicates are silently skipped.
 */
[[nodiscard]] std::expected<std::vector<std::string>, std::string>
resolve_input_files(const std::filesystem::path &source_dir, std::string_view files_str);

#endif // UTILS_HPP
