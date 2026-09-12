// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <map>
#include <ranges>
#include <set>
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

std::expected<int, std::string> parse_int(std::string_view value, std::string_view option_name) {
  int result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec == std::errc::result_out_of_range)
    return std::unexpected(std::format("Value out of range for {}: '{}'", option_name, value));
  if (ec != std::errc{} || ptr != value.data() + value.size())
    return std::unexpected(std::format("Invalid integer value for {}: '{}'", option_name, value));
  if (result < 0)
    return std::unexpected(std::format("{} must not be negative: '{}'", option_name, value));
  return result;
}

std::expected<double, std::string> parse_double(std::string_view value, std::string_view option_name) {
  double result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec == std::errc::result_out_of_range)
    return std::unexpected(std::format("Value out of range for {}: '{}'", option_name, value));
  if (ec != std::errc{} || ptr != value.data() + value.size())
    return std::unexpected(std::format("Invalid number for {}: '{}'", option_name, value));
  return result;
}

namespace {
  constexpr std::string_view k_full_canvas{"full-canvas"};
  constexpr double k_half{0.5};

  std::string invalid_pivot_message(std::string_view preset) {
    return std::format(
        "Invalid --pivot value '{}'. Use a preset (bottom-center, center, top-center, top-left) or "
        "full-canvas with an optional value (full-canvas, full-canvas:0.18, full-canvas:0.5,0.18)",
        preset);
  }
} // namespace

std::expected<Pivot, std::string> resolve_pivot(std::string_view preset) {
  if (preset == "bottom-center")
    return Pivot{k_half, 1.0, false};
  if (preset == "center")
    return Pivot{k_half, k_half, false};
  if (preset == "top-center")
    return Pivot{k_half, 0.0, false};
  if (preset == "top-left")
    return Pivot{0.0, 0.0, false};

  if (preset.starts_with(k_full_canvas)) {
    if (preset == k_full_canvas)
      return Pivot{k_half, 0.0, true}; // full-canvas bottom-center

    const std::string_view suffix{preset.substr(k_full_canvas.size())};
    if (!suffix.starts_with(':'))
      return std::unexpected(invalid_pivot_message(preset));
    const std::string_view value{suffix.substr(1)};

    double px{k_half}, py{0.0};
    if (const auto comma = value.find(','); comma != std::string_view::npos) {
      auto x = parse_double(value.substr(0, comma), "--pivot");
      if (!x)
        return std::unexpected(x.error());
      auto y = parse_double(value.substr(comma + 1), "--pivot");
      if (!y)
        return std::unexpected(y.error());
      px = *x;
      py = *y;
    } else {
      auto y = parse_double(value, "--pivot");
      if (!y)
        return std::unexpected(y.error());
      py = *y;
    }
    return Pivot{px, py, true};
  }

  return std::unexpected(invalid_pivot_message(preset));
}

std::expected<std::vector<std::string>, std::string> resolve_input_files(const std::filesystem::path &source_dir,
                                                                         std::string_view files_str) {
  constexpr std::string_view k_valid_extension = ".png";
  std::vector<std::string> input_files;
  std::unordered_set<std::string> seen;

  auto add_file = [&seen, &input_files](const std::filesystem::path &p) {
    const std::string s{p.string()};
    if (seen.insert(s).second)
      input_files.push_back(s);
  };

  auto ext_matches = [&k_valid_extension](const std::filesystem::path &p) {
    auto ext = p.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == k_valid_extension;
  };

  if (files_str.empty()) {
    std::vector<std::filesystem::path> matches;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(source_dir, ec)) {
      std::error_code status_ec;
      if (entry.is_regular_file(status_ec) && !status_ec && ext_matches(entry.path()))
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
      std::error_code exact_ec;
      if (std::filesystem::exists(exact) && std::filesystem::is_regular_file(exact, exact_ec) && !exact_ec) {
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
          std::error_code status_ec;
          if (!entry.is_regular_file(status_ec) || status_ec)
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

std::expected<void, std::string> validate_animation_naming(const std::vector<std::string> &files) {
  // group key: "<unit>_<state>", facing -> set of frame indices present for that facing.
  std::map<std::string, std::map<std::string, std::set<int>>> groups;

  for (const auto &file : files) {
    const std::string stem = std::filesystem::path(file).stem().string();
    std::vector<std::string_view> tokens;
    for (auto part : stem | std::views::split('_'))
      tokens.emplace_back(part.begin(), part.end());
    if (tokens.size() != 4)
      continue;

    int frame{};
    const std::string_view frame_tok{tokens[3]};
    const auto [ptr, ec] = std::from_chars(frame_tok.data(), frame_tok.data() + frame_tok.size(), frame);
    if (ec != std::errc{} || ptr != frame_tok.data() + frame_tok.size())
      continue;

    const std::string group_key{std::format("{}_{}", tokens[0], tokens[1])};
    const std::string facing{tokens[2]};
    groups[group_key][facing].insert(frame);
  }

  std::vector<std::string> issues;
  for (const auto &[group_key, facings] : groups) {
    if (facings.size() < 2)
      continue;

    std::set<int> reference;
    for (const auto &[facing, frames] : facings)
      reference.insert(frames.begin(), frames.end());

    for (const auto &[facing, frames] : facings) {
      std::vector<int> missing;
      std::ranges::set_difference(reference, frames, std::back_inserter(missing));
      if (!missing.empty()) {
        std::string missing_list;
        for (std::size_t i = 0; i < missing.size(); ++i)
          missing_list += (i == 0 ? "" : ", ") + std::to_string(missing[i]);
        issues.push_back(std::format("{}: facing '{}' is missing frame(s) {} (present in other facings)", group_key,
                                     facing, missing_list));
      }
    }
  }

  if (!issues.empty()) {
    std::string message{"Animation validation failed:"};
    for (const auto &issue : issues)
      message += std::format("\n  {}", issue);
    return std::unexpected(message);
  }
  return {};
}
