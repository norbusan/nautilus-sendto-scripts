plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -I$(builddir)				\
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(BLUETOOTH_CFLAGS)			\
	$(DBUS_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstbluetooth.la

bluetooth-marshal.h: bluetooth-marshal.list Makefile
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --header > $@

bluetooth-marshal.c: bluetooth-marshal.list Makefile
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --header --body > $@
bluetooth.c: bluetooth-marshal.c bluetooth-marshal.h

BUILT_SOURCES = bluetooth-marshal.h bluetooth-marshal.c
libnstbluetooth_la_SOURCES = bluetooth.c $(BUILT_SOURCES)
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

EXTRA_DIST = bluetooth-marshal.list bluetooth.c
CLEANFILES = $(BUILT_SOURCES)

