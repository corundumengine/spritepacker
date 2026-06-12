#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <expected>
#include <string>

/** Raw CLI arguments parsed directly from argv, before validation or type conversion. */
struct Options {
  std::string input;
  std::string files; // comma-separated filenames or glob patterns; empty means all PNGs
  std::string output;
  std::string name;     // base name for output files, e.g. "terrain" → terrain.png, terrain.json
  std::string size;     // optional frame size in "WxH" format
  std::string max_size; // optional maximum atlas size in "WxH" format
  bool show_help{false};
  bool show_version{false};

  /** Checks that source, output, and name are non-empty. */
  [[nodiscard]] std::expected<void, std::string> validate() const;

  /**
   * Parses command-line arguments into an Options struct. Returns immediately
   * with show_help set when --help/-h is passed or no arguments are provided.
   * Returns an error for unknown arguments or missing required fields.
   */
  [[nodiscard]] static std::expected<Options, std::string> parse_args(int argc, char *argv[]);
};

#endif // OPTIONS_HPP
