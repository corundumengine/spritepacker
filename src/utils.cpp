#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <ranges>
#include <system_error>
#include <unordered_set>

std::regex glob_to_regex(std::string_view pattern) {
  std::string re;
  re.reserve(pattern.size() * 2);
  static constexpr std::string_view k_regex_meta = R"((){}+|\^$)";

  bool in_bracket = false;
  bool next_is_bracket_start = false;
  std::size_t bracket_chars = 0;

  for (char c : pattern) {
    const bool is_bracket_start{next_is_bracket_start};
    next_is_bracket_start = false;

    if (in_bracket) {
      if (c == ']') {
        re += ']';
        in_bracket = false;
      } else if (c == '!' && is_bracket_start) {
        re += '^';
        ++bracket_chars;
      } else {
        re += c;
        ++bracket_chars;
      }
    } else {
      switch (c) {
      case '*':
        re += ".*";
        break;
      case '?':
        re += '.';
        break;
      case '.':
        re += "\\.";
        break;
      case '[':
        re += '[';
        in_bracket = true;
        next_is_bracket_start = true;
        bracket_chars = 0;
        break;
      default:
        if (k_regex_meta.find(c) != std::string_view::npos)
          re += '\\';
        re += c;
        break;
      }
    }
  }

  if (in_bracket) {
    if (bracket_chars == 0) {
      re.pop_back();
      re += "\\[";
    } else {
      re += ']';
    }
  }
  return std::regex{re, std::regex::icase};
}

std::expected<std::tuple<int, int>, std::string> parse_size(std::string_view size_str) {
  const auto pos = size_str.find('x');
  if (pos == std::string_view::npos) {
    return std::unexpected(std::format("Invalid size format '{}'. Use 'WxH' (e.g., '64x64')", size_str));
  }
  int w{}, h{};
  const char *begin{size_str.data()};
  const char *mid{begin + pos};
  const char *end{begin + size_str.size()};

  auto [p1, ec1] = std::from_chars(begin, mid, w);
  if (ec1 == std::errc::result_out_of_range)
    return std::unexpected(std::format("Size value out of range in '{}'", size_str));
  if (ec1 != std::errc{} || p1 != mid)
    return std::unexpected(std::format("Invalid size format '{}'. Use 'WxH' (e.g., '64x64')", size_str));

  auto [p2, ec2] = std::from_chars(mid + 1, end, h);
  if (ec2 == std::errc::result_out_of_range)
    return std::unexpected(std::format("Size value out of range in '{}'", size_str));
  if (ec2 != std::errc{} || p2 != end)
    return std::unexpected(std::format("Invalid size format '{}'. Use 'WxH' (e.g., '64x64')", size_str));

  if (w <= 0 || h <= 0)
    return std::unexpected(std::format("Size values must be positive in '{}'", size_str));
  return std::tuple{w, h};
}

std::expected<std::vector<std::string>, std::string> resolve_input_files(const std::filesystem::path &source_dir,
                                                                         std::string_view files_str) {
  constexpr std::string_view k_valid_extension = ".png";
  std::vector<std::string> input_files;
  std::unordered_set<std::string> seen;

  auto add_file = [&](const std::filesystem::path &p) {
    const std::string s{p.string()};
    if (seen.insert(s).second)
      input_files.push_back(s);
  };

  auto ext_matches = [&](const std::filesystem::path &p) {
    auto ext = p.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == k_valid_extension;
  };

  if (files_str.empty()) {
    std::vector<std::filesystem::path> matches;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(source_dir, ec)) {
      if (entry.is_regular_file() && ext_matches(entry.path()))
        matches.push_back(entry.path());
    }
    if (ec)
      return std::unexpected(std::format("Cannot read directory '{}': {}", source_dir.string(), ec.message()));
    std::ranges::sort(matches, {}, [](const std::filesystem::path &p) { return p.filename().string(); });
    for (const auto &p : matches)
      add_file(p);
  } else {
    for (auto token_range : files_str | std::views::split(',')) {
      std::string_view token(token_range.begin(), token_range.end());
      const auto first = token.find_first_not_of(" \t");
      if (first == std::string_view::npos)
        continue;
      const auto last = token.find_last_not_of(" \t");
      token = token.substr(first, last - first + 1);

      const std::filesystem::path exact = source_dir / token;
      if (std::filesystem::exists(exact) && std::filesystem::is_regular_file(exact)) {
        if (!ext_matches(exact)) {
          return std::unexpected(
              std::format("File '{}' is not a PNG file (extension: {})", token, exact.extension().string()));
        }
        add_file(exact);
      } else {
        const std::regex re{glob_to_regex(token)};
        std::vector<std::filesystem::path> matches;
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(source_dir, ec)) {
          if (!entry.is_regular_file())
            continue;
          if (ext_matches(entry.path()) && std::regex_match(entry.path().filename().string(), re))
            matches.push_back(entry.path());
        }
        if (ec)
          return std::unexpected(std::format("Cannot read directory '{}': {}", source_dir.string(), ec.message()));
        std::ranges::sort(matches, {}, [](const std::filesystem::path &p) { return p.filename().string(); });
        for (const auto &p : matches)
          add_file(p);
      }
    }
  }

  if (input_files.empty()) {
    return std::unexpected(std::format("No PNG files found in '{}'{}", source_dir.string(),
                                       files_str.empty() ? "" : std::format(" matching '{}'", files_str)));
  }
  return input_files;
}
