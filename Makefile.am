# Extra compiler flags and include paths
AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) $(GTOP_CFLAGS) $(YAML_CFLAGS)

# Legacy Automake setting for automatic handling of meta sources
METASOURCES = AUTO

# Build and install this binary program
bin_PROGRAMS = g15stats

# Test suite
TESTS = tests/test_yaml_config.sh tests/test_output_file_mode.sh tests/test_config_variables.sh
EXTRA_DIST = $(TESTS) tests/convert_g15_frames.py tests/convert_g15_frames.c tests/gen_screen_mode_dumps.sh tests/update_screens_docs.sh contrib/g15stats.yaml.example

# Header file used during build, but not installed
noinst_HEADERS = g15stats.h

# Documentation installation directory and files
docdir = @docdir@
doc_DATA = README.md AUTHORS COPYING

# Libraries to link against
g15stats_LDADD = $(GTOP_LIBS) $(YAML_LIBS) -lg15daemon_client -lg15render -lpthread -lm -lrt

# Linker flags for g15stats
# Note: -avoid-version is usually used for libraries, not programs
g15stats_LDFLAGS = -avoid-version

# Source files for the g15stats program
g15stats_SOURCES = g15stats.c g15stats.h

# Create a .tmp directory as part of the default build
all-local: $(top_builddir)/.tmp

# Rule to create the temporary build directory
$(top_builddir)/.tmp:
	mkdir -p $(top_builddir)/.tmp

# Lint reports (report-only, never fail build)
lint: lint-cppcheck lint-clang-tidy

lint-cppcheck: $(top_builddir)/.tmp
	@mkdir -p "$(top_builddir)/.tmp/lint"
	@printf "Running cppcheck (report-only)...\n"
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=warning,style,performance,portability --inconclusive --std=c99 --quiet \
			-I"$(top_srcdir)" g15stats.c 2>"$(top_builddir)/.tmp/lint/cppcheck.txt" || true; \
		printf "cppcheck report: %s\n" "$(top_builddir)/.tmp/lint/cppcheck.txt"; \
	else \
		printf "cppcheck not found - skipped\n" >"$(top_builddir)/.tmp/lint/cppcheck.txt"; \
		printf "cppcheck not found, skipped\n"; \
	fi

lint-clang-tidy: $(top_builddir)/.tmp
	@mkdir -p "$(top_builddir)/.tmp/lint"
	@printf "Running clang-tidy (report-only)...\n"
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy g15stats.c -- $(AM_CPPFLAGS) $(CPPFLAGS) >"$(top_builddir)/.tmp/lint/clang-tidy.txt" 2>&1 || true; \
		printf "clang-tidy report: %s\n" "$(top_builddir)/.tmp/lint/clang-tidy.txt"; \
	else \
		printf "clang-tidy not found - skipped\n" >"$(top_builddir)/.tmp/lint/clang-tidy.txt"; \
		printf "clang-tidy not found, skipped\n"; \
	fi

screens-docs:
	@tests/update_screens_docs.sh
