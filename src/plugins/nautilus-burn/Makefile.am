plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -I$(srcdir)/../				\
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(WARN_CFLAGS)				\
	$(GIO_CFLAGS)

plugin_LTLIBRARIES = libnstburn.la

libnstburn_la_SOURCES = nautilus-burn.c
libnstburn_la_LDFLAGS = -module -avoid-version
libnstburn_la_LIBADD = $(GIO_LIBS) $(builddir)/../libnstcommon.la

