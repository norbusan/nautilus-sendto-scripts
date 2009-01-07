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

plugin_LTLIBRARIES = libnstthunderbird.la

libnstthunderbird_la_SOURCES = thunderbird.c
libnstthunderbird_la_LDFLAGS = -module -avoid-version
#libnstthunderbird_la_LIBADD = $(THUNDERBIRD_LIBS)

