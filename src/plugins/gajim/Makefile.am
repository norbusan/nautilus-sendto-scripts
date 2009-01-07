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

plugin_LTLIBRARIES = libnstgajim.la

libnstgajim_la_SOURCES = gajim.c
libnstgajim_la_LDFLAGS = -module -avoid-version
libnstgajim_la_LIBADD = $(DBUS_LIBS)

