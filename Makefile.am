# Extra C compiler flags:
# -Wall enables common warnings
# @CFLAGS@ is substituted by configure
# Include both the top build directory and top source directory
AM_CFLAGS = -Wall @CFLAGS@ -I$(top_builddir) -I$(top_srcdir)

# Legacy Automake setting for automatic handling of meta sources
METASOURCES = AUTO

# Build and install this binary program
bin_PROGRAMS = g15stats

# Header file used during build, but not installed
noinst_HEADERS = g15stats.h

# Documentation installation directory and files
docdir = @docdir@
doc_DATA = README ChangeLog AUTHORS NEWS COPYING

# Libraries to link against
LIBS = -lg15daemon_client -lg15render -lpthread -lm -lrt

# Additional include flags
# Note: this looks incomplete and may be a leftover
INCLUDES = -I

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