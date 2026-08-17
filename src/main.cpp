#include "options.hpp"
#include "sprite_packer.hpp"
#include "version.hpp"

#include <cstdlib>
#include <format>
#include <print>

void print_usage(const char *program_name) {
  std::println(R"(spritepacker v{1}
Packs PNG sprites from a source directory into one or more atlas sheets using a MaxRects
bin-packer, trimming transparent padding and deduplicating identical frames.

Usage: {0} --input <dir> --sheets <dir> --name <name> [options]

Options:
  --input,      -i <dir>   Source directory containing PNG files
  --sheets      <dir>      Output directory for JSON sheet metadata
  --assets      <dir>      Output directory for PNG sprite sheets (default: same as --sheets)
  --name,       -n <name>  Base name for output (e.g., "terrain" → terrain.png, terrain.json)
  --files,      -f <list>  Comma-separated filenames or wildcard patterns (default: *.png)
  --max-size,   -m WxH     Maximum atlas size per sheet (default: 2048x2048)
  --padding,    -p <n>     Pixel gap between packed sprites (default: 1)
  --pivot       <preset>   Sprite anchor point. Presets anchor the trimmed art: bottom-center
                           (default), center, top-center, top-left. "full-canvas" anchors the FULL
                           source canvas (y from bottom), preserving source padding, and takes an
                           optional value: full-canvas, full-canvas:0.18, full-canvas:0.5,0.18
  --validate-animations    Require <unit>_<state>_<facing>_<frame>.png sets to have matching frames
                            across all facings of the same animation; fails the build if not
  --pot                    Round each sheet's final width/height up to the next power of two
  --version,    -v         Show version
  --help,       -h         Show this message

File selection notes:
  --files accepts exact filenames or wildcard patterns (* and ? supported), separated by commas.
  If omitted, all PNG files in --input are included.
  Exact filenames are added in the order listed; wildcard matches are sorted alphabetically.

Notes:
  Compressed texture output (ASTC/ETC/BCn) is out of scope for this tool; sheets are written as
  PNG and expected to be compressed by a later build step if the target platform needs it.
  Depth-sort order is not written separately — an isometric engine should derive it from each
  sprite's pivot_y (bottom-center's pivot_y == 1.0 is the sprite's "feet").

Examples:
  {0} --input tiles/terrain --sheets game/data/sprite_sheets --name terrain
  {0} --input tiles/objects --files chest_open.png,chest_closed.png,chest_gold.png --sheets game/data/sprite_sheets --name chests
  {0} -i tiles/objects -f chest_*.png --sheets game/data/sprite_sheets -n chests -p 2
  {0} -i tiles/ --sheets metadata/ --assets assets/textures/ -n terrain --validate-animations
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

  std::println("Creating {} sheet(s) in {}...", pack_data->num_sheets, pack_data->assets_dir.string());
  if (auto result = pack_data->pack(); !result) {
    std::println(stderr, "Error: {}", result.error());
    return EXIT_FAILURE;
  }
  std::println("Packed {} sprites into {} sheet(s).", pack_data->sprites.size(), pack_data->num_sheets);

  if (auto result = pack_data->write_metadata(); !result) {
    std::println(stderr, "Error: {}", result.error());
    return EXIT_FAILURE;
  }
  std::println("Wrote metadata files in {}.", pack_data->sheets_dir.string());

  std::println("Done!");

  return EXIT_SUCCESS;
}
