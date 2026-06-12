#include "options.hpp"

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

  static constexpr OptSpec OPTIONS[] = {
      {"--help", "-h", false, [](Options &o, std::string_view) { o.show_help = true; }},
      {"--version", "-v", false, [](Options &o, std::string_view) { o.show_version = true; }},
      {"--input", "-i", true, [](Options &o, std::string_view v) { o.input = v; }},
      {"--files", "-f", true, [](Options &o, std::string_view v) { o.files = v; }},
      {"--output", "-o", true, [](Options &o, std::string_view v) { o.output = v; }},
      {"--name", "-n", true, [](Options &o, std::string_view v) { o.name = v; }},
      {"--size", "-s", true, [](Options &o, std::string_view v) { o.size = v; }},
      {"--max-size", "-m", true, [](Options &o, std::string_view v) { o.max_size = v; }},
  };
} // namespace

std::expected<void, std::string> Options::validate() const {
  if (input.empty())
    return std::unexpected("Missing required argument: --input");
  if (output.empty())
    return std::unexpected("Missing required argument: --output");
  if (name.empty())
    return std::unexpected("Missing required argument: --name");
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
