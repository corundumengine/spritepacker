// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "options.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <expected>
#include <format>
#include <span>
#include <string_view>

namespace {
  struct OptSpec {
    std::string_view long_name;
    std::string_view short_name;
    bool has_value;
    void (*set_value)(Options &, std::string_view);
  };

  constexpr std::array<OptSpec, 12> k_options{
      {
          {
              .long_name = "--help",
              .short_name = "-h",
              .has_value = false,
              .set_value = [](Options &o, std::string_view) { o.show_help = true; },
          },
          {
              .long_name = "--version",
              .short_name = "-v",
              .has_value = false,
              .set_value = [](Options &o, std::string_view) { o.show_version = true; },
          },
          {
              .long_name = "--input",
              .short_name = "-i",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.input = v; },
          },
          {
              .long_name = "--files",
              .short_name = "-f",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.files = v; },
          },
          {
              .long_name = "--sheets",
              .short_name = "",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.sheets = v; },
          },
          {
              .long_name = "--assets",
              .short_name = "",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.assets = v; },
          },
          {
              .long_name = "--name",
              .short_name = "-n",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.name = v; },
          },
          {
              .long_name = "--max-size",
              .short_name = "-m",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.max_size = v; },
          },
          {
              .long_name = "--padding",
              .short_name = "-p",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.padding = v; },
          },
          {
              .long_name = "--pivot",
              .short_name = "",
              .has_value = true,
              .set_value = [](Options &o, std::string_view v) { o.pivot = v; },
          },
          {
              .long_name = "--validate-animations",
              .short_name = "",
              .has_value = false,
              .set_value = [](Options &o, std::string_view) { o.validate_animations = true; },
          },
          {
              .long_name = "--pot",
              .short_name = "",
              .has_value = false,
              .set_value = [](Options &o, std::string_view) { o.pot = true; },
          },
      },
  };

  const OptSpec *find_option(std::string_view arg) {
    const auto match = [arg](const OptSpec &opt) { return arg == opt.long_name || arg == opt.short_name; };
    const auto *const it = std::ranges::find_if(k_options, match);
    return it == k_options.end() ? nullptr : &*it;
  }
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

std::expected<Options, std::string> Options::parse_args(std::span<char *> argv) {
  Options opts;

  // No arguments: show help
  if (argv.size() == 1) {
    opts.show_help = true;
    return opts;
  }

  // Skip the program name
  const auto args = argv.subspan(1);

  // Parse each argument
  for (auto it = args.begin(); it != args.end(); ++it) {
    const std::string_view arg{*it};

    const OptSpec *opt = find_option(arg);
    if (opt == nullptr)
      return std::unexpected(std::format("Unknown argument '{}'", arg));

    if (opt->has_value) {
      if (++it == args.end())
        return std::unexpected(std::format("Missing value for {}", opt->long_name));
      if (std::string_view{*it}.starts_with('-'))
        return std::unexpected(
            std::format("Unexpected flag '{}' where a value for {} was expected", *it, opt->long_name));
      opt->set_value(opts, *it);
    } else {
      opt->set_value(opts, {});
    }

    if (opts.show_help || opts.show_version)
      return opts;
  }

  if (auto result = opts.validate(); !result)
    return std::unexpected(result.error());

  return opts;
}
