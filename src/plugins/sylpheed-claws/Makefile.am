plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstsylpheed.la

libnstsylpheed_la_SOURCES = sylpheed-claws.c
libnstsylpheed_la_LDFLAGS = -module -avoid-version
#libnstsylpheed_la_LIBADD = $(SYLPHEED_LIBS)

