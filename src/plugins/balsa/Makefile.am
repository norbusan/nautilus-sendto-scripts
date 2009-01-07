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

plugin_LTLIBRARIES = libnstbalsa.la

libnstbalsa_la_SOURCES = balsa.c
libnstbalsa_la_LDFLAGS = -module -avoid-version
#libnstbalsa_la_LIBADD = $(BALSA_LIBS)

