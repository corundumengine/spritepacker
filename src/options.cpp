// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "options.hpp"
#include "utils.hpp"

#include <format>
#include <ranges>
#include <span>
#include <string_view>

namespace {
  struct OptSpec {
    std::string_view long_name;
    std::string_view short_name;
    bool has_value;
    void (*set_value)(Options &, std::string_view);
  };

  constexpr OptSpec OPTIONS[] = {
      {"--help", "-h", false, [](Options &o, std::string_view) { o.show_help = true; }},
      {"--version", "-v", false, [](Options &o, std::string_view) { o.show_version = true; }},
      {"--input", "-i", true, [](Options &o, std::string_view v) { o.input = v; }},
      {"--files", "-f", true, [](Options &o, std::string_view v) { o.files = v; }},
      {"--sheets", "", true, [](Options &o, std::string_view v) { o.sheets = v; }},
      {"--assets", "", true, [](Options &o, std::string_view v) { o.assets = v; }},
      {"--name", "-n", true, [](Options &o, std::string_view v) { o.name = v; }},
      {"--max-size", "-m", true, [](Options &o, std::string_view v) { o.max_size = v; }},
      {"--padding", "-p", true, [](Options &o, std::string_view v) { o.padding = v; }},
      {"--pivot", "", true, [](Options &o, std::string_view v) { o.pivot = v; }},
      {"--validate-animations", "", false, [](Options &o, std::string_view) { o.validate_animations = true; }},
      {"--pot", "", false, [](Options &o, std::string_view) { o.pot = true; }},
  };
} // namespace

std::expected<void, std::string> Options::validate() const {
  if (input.empty())
    return std::unexpected("Missing required argument: --input");
  if (sheets.empty())
    return std::unexpected("Missing required argument: --sheets");
  if (name.empty())
    return std::unexpected("Missing required argument: --name");
  if (auto result = resolve_pivot(pivot); !result)
    return std::unexpected(result.error());
  return {};
}

std::expected<Options, std::string> Options::parse_args(int argc, char *argv[]) {
  Options opts;

  // No arguments: show help
  if (argc == 1) {
    opts.show_help = true;
    return opts;
  }

  // Skip the program name
  const auto args = std::span(argv, static_cast<std::size_t>(argc)) | std::views::drop(1);

  // Parse each argument
  for (auto it = args.begin(); it != args.end(); ++it) {
    const std::string_view arg{*it};
    bool matched = false;

    // Look for a matching option
    for (const auto &opt : OPTIONS) {
      if (arg != opt.long_name && arg != opt.short_name)
        continue;

      matched = true;
      if (opt.has_value) {
        if (++it == args.end())
          return std::unexpected(std::format("Missing value for {}", opt.long_name));
        if (std::string_view{*it}.starts_with('-'))
          return std::unexpected(
              std::format("Unexpected flag '{}' where a value for {} was expected", *it, opt.long_name));
        opt.set_value(opts, *it);
      } else {
        opt.set_value(opts, {});
      }

      // Go to the next argument after a match
      break;
    }

    if (!matched)
      return std::unexpected(std::format("Unknown argument '{}'", arg));

    if (opts.show_help || opts.show_version)
      return opts;
  }

  if (auto result = opts.validate(); !result)
    return std::unexpected(result.error());

  return opts;
}
