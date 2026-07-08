#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <expected>
#include <string>

/** Raw CLI arguments parsed directly from argv, before validation or type conversion. */
struct Options {
  std::string input;
  std::string files; // comma-separated filenames or glob patterns; empty means all PNGs
  std::string sheets;
  std::string assets;
  std::string name;                   // base name for output files, e.g. "terrain" → terrain.png, terrain.json
  std::string max_size;               // optional maximum atlas size in "WxH" format
  std::string padding;                // optional pixel gap between packed sprites (default: "1")
  std::string pivot{"bottom-center"}; // preset: bottom-center, center, top-center, top-left
  std::string pivot_manifest;         // optional path to a JSON file of per-sprite pivot overrides
  bool validate_animations{false};    // if set, enforce <unit>_<state>_<facing>_<frame>.png completeness
  bool pot{false};                    // round each sheet's final dimensions up to the next power of two
  bool show_help{false};
  bool show_version{false};

  /** Checks that source, sheets, and name are non-empty. */
  [[nodiscard]] std::expected<void, std::string> validate() const;

  /**
   * Parses command-line arguments into an Options struct. Returns immediately
   * with show_help set when --help/-h is passed or no arguments are provided.
   * Returns an error for unknown arguments or missing required fields.
   */
  [[nodiscard]] static std::expected<Options, std::string> parse_args(int argc, char *argv[]);
};

#endif // OPTIONS_HPP
