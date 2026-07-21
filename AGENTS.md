# AGENTS.md

This file contains information for agentic coding agents operating in this repository.

## Project Overview
G15Stats is a C system-stats monitor for Logitech G15/G15v2 LCD displays (via
G15Daemon). It renders CPU, memory, swap, network, GPU, temperature, fan, and
battery screens. Built with autotools; requires `libgtop-2.0`,
`libg15daemon_client`, `libg15render`, and `libyaml` development packages.

## Build Commands
```bash
# Generate configure/Makefile.in (git checkout only; not needed from a
# release tarball, and not checked into git — requires autoconf, automake)
autoreconf -fi

# Configure and build
./configure
make -j"$(nproc)"

# Clean build
make clean
make distclean

# Install
sudo make install
```

## Lint Commands
Report-only targets; reports are written to `.tmp/lint/` and never fail the build:
```bash
make lint-cppcheck
make lint-clang-tidy
make lint
```

## Test Commands
```bash
make check
```
Tests are plain bash scripts under `tests/`, registered in `Makefile.am`'s
`TESTS` variable (`test_yaml_config.sh`, `test_output_file_mode.sh`,
`test_config_variables.sh`). To run a single test directly, build first, then
invoke the script from the repo root (it locates the `g15stats` binary via
`abs_top_builddir`, defaulting to `$(pwd)`):
```bash
make
./tests/test_config_variables.sh
```
Tests run `g15stats` with `G15STATS_FORCE_SCREEN`, `G15STATS_FORCE_MODE`, and
`G15STATS_PAUSE` env vars plus `-o <file>` (output-file mode) to render frames
without a real G15 keyboard/daemon, then diff/inspect the raw frame output.
`G15STATS_CONFIG_FILE` overrides the YAML config path for the same reason.

## Architecture

`g15stats.c` is a single ~5000-line file organized into clearly marked
sections (search for `* Section name` after the `====` banners):
global state, configuration, screen metadata, utility functions, hardware/
sensor probing, info label printers, screen renderers, info cycling/label
dispatch, overlays, input handling and background threads, and the entry
point. `g15stats.h` holds shared constants (`SCREEN_*` IDs, buffer sizes,
`BAR_START`/`BAR_END`/`BAR_BOTTOM` LCD geometry) and the two small structs
used for sensor/battery data.

- **Screens**: identified by `SCREEN_*` defines (`SCREEN_SUMMARY` = 0 through
  `SCREEN_MEM_PRESSURE` = 9, `MAX_SCREENS` = 9). Each has a
  `draw_*_screen()` renderer called from the main loop based on the global
  `cycle` variable. Screens can have a `mode[cycle]` (toggled with L4,
  e.g. bar view vs. vertical view) and some react to a global `submode`
  (L5). `screen_name()`/`screen_visible()` and friends centralize
  screen metadata; add new screens there and in the `draw_*` dispatch,
  not by special-casing call sites.
- **Config precedence**: CLI args > `$G15STATS_CONFIG_FILE` (if set) >
  `~/.config/g15stats/g15stats.yaml` > `/etc/g15plugins/g15stats.yaml`. A
  default user config is written on first run if none exists. See the
  "Configuration" section header in `g15stats.c` for the loader.
- **Threading**: `keyboard_watch()` runs on the main thread and processes
  G15 key events (L1-L5), updating `cycle`/`mode[]`/`submode`.
  `network_watch()` and `auto_discover_nic()` run on a dedicated thread to
  keep network stats current independent of the render loop.
- **Rendering**: screens draw into a `g15canvas` via `libg15render`
  (`g15r_*` calls) and the frame is pushed to `g15daemon` over a socket
  (`g15screen_fd`), unless `-o <file>` / `output_file` redirects raw frames
  to a file instead (used for headless testing).
- **Hardware probing** degrades gracefully: features like `have_freq`,
  `have_temp`, `have_fan`, `have_bat`, `have_gpu`, `have_mem_pressure` are
  probed once (or re-probed under `variable_cpu`) and gate both screen
  visibility and per-frame work.
- **Debug logging**: gated behind the `_Bool debug_enabled` global (`-D`/
  `--debug` or config `debug: true`), emitted via `fprintf(stderr, ...)`.
  There is no dedicated logging wrapper.

## Code Style Guidelines
- GNU-style C, 4-space indentation, no tabs.
- Functions/variables: `lowercase_with_underscores`; constants:
  `UPPERCASE_WITH_UNDERSCORES`.
- Global state uses plain globals declared in the "Global state" section of
  `g15stats.c` (no `extern` header, no wrapper accessors) — this is the
  established pattern, not something to refactor away incidentally.
- `_Bool` for flags, `int`/`unsigned int` for counts, `const` for
  non-modifiable params/returns.

## Documentation
`README.md` and `docs/` (MkDocs) are user-facing. Screen renderer functions
have short header comments describing what they draw; there are no
docstring-style comments elsewhere. Browse rendered docs with:
```bash
uv run mkdocs serve
```

## Additional agent notes
- See `.AGENTS/main_files.md` for a quick file map, and
  `.AGENTS/mkdocs_material_reference/` for MkDocs Material authoring
  reference when editing `docs/`.
- Avoid editing files in `.git`; be cautious changing the global screen
  state variables (`cycle`, `mode[]`, `submode`) since input handling,
  info-cycling, and every screen renderer all read them.
- Before committing: `make` (compiles clean) and `make check` (tests pass).
