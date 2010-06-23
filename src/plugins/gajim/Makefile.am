plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NAUTILUS_SENDTO_CFLAGS)               \
        $(DBUS_CFLAGS)				\
        $(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstgajim.la

libnstgajim_la_SOURCES = gajim.c
libnstgajim_la_LDFLAGS = -module -avoid-version
libnstgajim_la_LIBADD = $(DBUS_LIBS) $(NAUTILUS_SENDTO_LIBS)

