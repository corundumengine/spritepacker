// SPDX-FileCopyrightText: 2026 Gentle Lion Studios, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "options.hpp"
#include "sprite_packer.hpp"
#include "version.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <print>
#include <span>
#include <string>
#include <string_view>

namespace {

  constexpr int k_step_total{3};

  /** argv[0] without any leading directories, so Usage/Examples stay short. */
  std::string program_basename(const char *argv0) {
    if (argv0 == nullptr || *argv0 == '\0')
      return "spritepacker";
    const std::string name{std::filesystem::path{argv0}.filename().string()};
    return name.empty() ? "spritepacker" : name;
  }

  /** Counts UTF-8 code points (all of which are single-column here) for panel alignment. */
  std::size_t display_width(std::string_view text) {
    std::size_t width{0};
    for (const char c : text) {
      if ((static_cast<unsigned char>(c) & 0xC0U) != 0x80U)
        ++width;
    }
    return width;
  }

  std::string repeat(std::string_view unit, std::size_t count) {
    std::string out;
    out.reserve(unit.size() * count);
    for (std::size_t i = 0; i < count; ++i)
      out += unit;
    return out;
  }

  std::string_view plural(std::size_t count, std::string_view singular, std::string_view plural_form) {
    return count == 1 ? singular : plural_form;
  }

  void print_banner() {
    std::println("  ▄▀ SPRITEPACKER {}  ·  sprite atlas packer", SPRITEPACKER_VERSION);
    std::println();
  }

  void print_usage(const std::string &program_name) {
    print_banner();
    std::println(R"(USAGE
  {0} -i <dir> -n <name> --sheets <dir> [options]

INPUT
  -i, --input <dir>           Source directory of PNG files               (required)
  -f, --files <list>          Comma-separated filenames or glob patterns  (default: *.png)

OUTPUT
      --sheets <dir>          JSON metadata output directory              (required)
      --assets <dir>          PNG sheet output directory                  (default: --sheets)
  -n, --name <name>           Base name: terrain -> terrain.png + .json   (required)

LAYOUT
  -m, --max-size <WxH>        Maximum atlas size per sheet                (default: 2048x2048)
  -p, --padding <n>           Pixel gap between packed sprites            (default: 1)
      --pivot <preset>        Anchor: bottom-center (default), center, top-center,
                              top-left, or full-canvas[:Y] / full-canvas:X,Y
      --pot                   Round sheet dimensions up to a power of two

BEHAVIOR
      --validate-animations   Fail if an animation's facings have mismatched frames
  -h, --help                  Show this help
  -v, --version               Show version

EXAMPLES
  {0} -i tiles/terrain -n terrain --sheets game/data/sprite_sheets
  {0} -i tiles/objects -f 'chest_*.png' -n chests --sheets out -p 2
  {0} -i tiles -n terrain --sheets metadata --assets assets/textures --validate-animations

Sprites are trimmed of transparent padding and deduplicated by pixel content. Sheets are
written as PNG; compressed formats (ASTC/ETC/BCn) are a later build step. See README.md.
)",
                 program_name);
  }

  void print_step(int step, std::string_view label, std::string_view detail) {
    std::println("  [{}/{}] {:<6} {}", step, k_step_total, label, detail);
  }

  void print_panel(std::string_view title, std::span<const std::string> lines) {
    std::size_t inner{display_width(title) + 1};
    for (const std::string &line : lines)
      inner = std::max(inner, display_width(line));

    std::println("  ╭─ {} {}╮", title, repeat("─", inner - display_width(title) - 1));
    for (const std::string &line : lines)
      std::println("  │ {}{} │", line, repeat(" ", inner - display_width(line)));
    std::println("  ╰{}╯", repeat("─", inner + 2));
  }

} // namespace

int main(int argc, char *argv[]) {
  const std::string program_name{program_basename(argc > 0 ? argv[0] : nullptr)};

  if (argc == 0) {
    std::println(stderr, "error: argc is zero");
    return EXIT_FAILURE;
  }

  auto options = Options::parse_args(std::span<char *>(argv, static_cast<std::size_t>(argc)));
  if (!options) {
    std::println(stderr, "error: {}", options.error());
    return EXIT_FAILURE;
  }

  if (options->show_version) {
    std::println("{}", SPRITEPACKER_VERSION);
    return EXIT_SUCCESS;
  }

  if (options->show_help) {
    print_usage(program_name);
    return EXIT_SUCCESS;
  }

  const auto start = std::chrono::steady_clock::now();
  print_banner();

  auto pack_data = PackData::from_options(*options);
  if (!pack_data) {
    std::println(stderr, "error: {}", pack_data.error());
    return EXIT_FAILURE;
  }

  const std::size_t sprite_count{pack_data->sprites.size()};
  const std::size_t unique_count{pack_data->unique_images.size()};
  const std::size_t duplicate_count{sprite_count - unique_count};
  const int sheet_count{pack_data->num_sheets};

  print_step(1, "PACK",
             std::format("{} {} ({} deduped) → {} {}", sprite_count, plural(sprite_count, "sprite", "sprites"),
                         duplicate_count, sheet_count, plural(sheet_count, "sheet", "sheets")));

  if (auto result = pack_data->pack(); !result) {
    std::println(stderr, "error: {}", result.error());
    return EXIT_FAILURE;
  }
  print_step(2, "SHEETS", std::format("{} png → {}", sheet_count, pack_data->assets_dir.string()));

  if (auto result = pack_data->write_metadata(); !result) {
    std::println(stderr, "error: {}", result.error());
    return EXIT_FAILURE;
  }
  print_step(3, "META", std::format("{} json → {}", sheet_count, pack_data->sheets_dir.string()));

  const double seconds{std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()};
  std::println();

  const std::string summary{
      std::format("{} {} → {} {}", sprite_count, plural(sprite_count, "sprite", "sprites"), sheet_count,
                  plural(sheet_count, "sheet", "sheets")),
  };
  const std::string stats{
      std::format("{} unique · {} deduped · {} files in {:.2f}s", unique_count, duplicate_count, sheet_count * 2,
                  seconds),
  };
  const std::array<std::string, 2> panel_lines{summary, stats};
  print_panel("done", panel_lines);

  return EXIT_SUCCESS;
}
