
AM_CFLAGS = -Wall @CFLAGS@ -I$(top_builddir) -I$(top_srcdir)
METASOURCES = AUTO
bin_PROGRAMS = g15stats
noinst_HEADERS = g15stats.h

docdir = @docdir@
doc_DATA = README ChangeLog AUTHORS NEWS COPYING

LIBS = -lg15daemon_client -lg15render -lpthread -lm -lrt
INCLUDES = -I
g15stats_LDFLAGS = -avoid-version
g15stats_SOURCES =  g15stats.c g15stats.h

# Create .tmp directory for temporary files during build
all-local: $(top_builddir)/.tmp

$(top_builddir)/.tmp:
	mkdir -p $(top_builddir)/.tmp


