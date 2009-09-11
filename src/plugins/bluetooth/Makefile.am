plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(BLUETOOTH_CFLAGS)			\
	$(DBUS_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstbluetooth.la

bluetooth-marshal.h: bluetooth-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --header > $@

bluetooth-marshal.c: bluetooth-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --body > $@
bluetooth.c: bluetooth-marshal.c bluetooth-marshal.h

libnstbluetooth_la_SOURCES = bluetooth.c bluetooth-marshal.h bluetooth-marshal.c
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

EXTRA_DIST = bluetooth-marshal.list
CLEANFILES = bluetooth-marshal.h bluetooth-marshal.c

