#include "options.hpp"
#include "sprite_packer.hpp"
#include "version.hpp"

#include <cstdlib>
#include <format>
#include <print>

void print_usage(const char *program_name) {
  std::println(R"(spritepacker v{1}
Packs PNG sprites from a source directory into one or more atlas sheets.

Usage: {0} --input <dir> --output <dir> --name <name> [--files <list>] [--size WxH] [--max-size WxH]

Options:
  --input,    -i <dir>   Source directory containing PNG files
  --output,   -o <dir>   Output directory for PNG and JSON files
  --name,     -n <name>  Base name for output (e.g., "terrain" → terrain.png, terrain.json)
  --files,    -f <list>  Comma-separated filenames or wildcard patterns (default: *.png)
  --size,     -s WxH     Frame size (e.g., "64x64"). Omit to auto-detect from input files.
  --max-size, -m WxH     Maximum atlas size (default: 2048x2048)
  --version,  -v         Show version
  --help,     -h         Show this message

File selection notes:
  --files accepts exact filenames or wildcard patterns (* and ? supported), separated by commas.
  If omitted, all PNG files in --input are included.
  Exact filenames are added in the order listed; wildcard matches are sorted alphabetically.

Examples:
  {0} --input tiles/terrain --output game/data/sprite_sheets --name terrain
  {0} --input tiles/objects --files chest_open.png,chest_closed.png,chest_gold.png --output game/data/sprite_sheets --name chests
  {0} -i tiles/objects -f chest_*.png -o game/data/sprite_sheets -n chests -s 32x32
)",
               program_name, SPRITEPACKER_VERSION);
}

int main(int argc, char *argv[]) {
  const char *const program_name{(argc > 0 && argv[0]) ? argv[0] : "spritepacker"};

  if (argc == 0) {
    std::println(stderr, "Error: argc is zero");
    return EXIT_FAILURE;
  }

  auto options = Options::parse_args(argc, argv);

  if (!options) {
    std::println(stderr, "Error: {}", options.error());
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

  auto pack_data = PackData::from_options(*options);
  if (!pack_data) {
    std::println(stderr, "Error: {}", pack_data.error());
    return EXIT_FAILURE;
  }

  std::println("Creating {} sheet(s) in {}...", pack_data->num_sheets, pack_data->output_dir.string());
  if (auto result = pack_data->pack(); !result) {
    std::println(stderr, "Error: {}", result.error());
    return EXIT_FAILURE;
  }
  std::println("Packed {} sprites into {} sheet(s).", pack_data->images.size(), pack_data->num_sheets);

  if (auto result = pack_data->write_metadata(); !result) {
    std::println(stderr, "Error: {}", result.error());
    return EXIT_FAILURE;
  }
  std::println("Wrote metadata files in {}.", pack_data->output_dir.string());

  std::println("Done!");

  return EXIT_SUCCESS;
}
