plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(NST_EBOOK_CFLAGS)			\
	$(GAIM_CFLAGS)				\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstgaim.la

libnstgaim_la_SOURCES = gaim.c
libnstgaim_la_LDFLAGS = -module -avoid-version
libnstgaim_la_LIBADD = $(GAIM_LIBS)

