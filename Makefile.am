# Extra compiler flags and include paths
AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) $(GTOP_CFLAGS) $(YAML_CFLAGS)

# Legacy Automake setting for automatic handling of meta sources
METASOURCES = AUTO

# Build and install this binary program
bin_PROGRAMS = g15stats

# Test suite
TESTS = tests/test_yaml_config.sh
EXTRA_DIST = $(TESTS)

# Header file used during build, but not installed
noinst_HEADERS = g15stats.h

# Documentation installation directory and files
docdir = @docdir@
doc_DATA = README.rst ChangeLog AUTHORS NEWS COPYING

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
