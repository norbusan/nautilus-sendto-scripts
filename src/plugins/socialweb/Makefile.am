plugindir = $(libdir)/nautilus-sendto/plugins

plugin_DATA = socialweb.nst-plugin

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -I$(srcdir)/../				\
        $(SOCIALWEB_CFLAGS)			\
        $(NAUTILUS_SENDTO_CFLAGS)               \
        $(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstsocialweb.la

libnstsocialweb_la_SOURCES = socialweb.c
libnstsocialweb_la_LDFLAGS = -module -avoid-version
libnstsocialweb_la_LIBADD = $(SOCIALWEB_LIBS) $(NAUTILUS_SENDTO_LIBS) $(builddir)/../libnautilussendto-1.0.la

