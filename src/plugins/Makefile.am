plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(NST_EBOOK_CFLAGS)			\
	$(GAIM_CFLAGS)				\
	$(BLUETOOTH_CFLAGS)			\
	$(DBUS_CFLAGS)				\
	$(WARN_CFLAGS)

if HAVE_GAIM
GAIM_PLUGIN = libnstgaim.la
else
GAIM_PLUGIN =
endif

if HAVE_BLUETOOTH
BLUETOOTH_PLUGIN = libnstbluetooth.la
else
BLUETOOTH_PLUGIN = 
endif

if HAVE_EVOLUTION
EVOLUTION_PLUGIN = libnstevolution.la
else
EVOLUTION_PLUGIN =
endif

if HAVE_BALSA
BALSA_PLUGIN = libnstbalsa.la
else
BALSA_PLUGIN = 
endif

if HAVE_SYLPHEED
SYLPHEED_PLUGIN = libnstsylpheed.la
else
SYLPHEED_PLUGIN = 
endif

if HAVE_THUNDERBIRD
THUNDERBIRD_PLUGIN = libnstthunderbird.la
else
THUNDERBIRD_PLUGIN = 
endif

if HAVE_GAJIM
GAJIM_PLUGIN = libnstgajim.la
else
GAJIM_PLUGIN = 
endif

plugin_LTLIBRARIES = $(EVOLUTION_PLUGIN) $(GAIM_PLUGIN) $(BLUETOOTH_PLUGIN) $(BALSA_PLUGIN) $(SYLPHEED_PLUGIN) $(THUNDERBIRD_PLUGIN) $(GAJIM_PLUGIN)

libnstevolution_la_SOURCES = evolution.c
libnstevolution_la_LDFLAGS = -module -avoid-version
libnstevolution_la_LIBADD = $(NST_EBOOK_LIBS)

libnstgaim_la_SOURCES = gaim.c
libnstgaim_la_LDFLAGS = -module -avoid-version
libnstgaim_la_LIBADD = $(GAIM_LIBS)

bluetooth-marshal.h: bluetooth-marshal.list
	$(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --header > $@

bluetooth-marshal.c: bluetooth-marshal.list
	$(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --body > $@
bluetooth.c: bluetooth-marshal.c bluetooth-marshal.h

libnstbluetooth_la_SOURCES = bluetooth.c bluetooth-marshal.h bluetooth-marshal.c
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

libnstbalsa_la_SOURCES = balsa.c
libnstbalsa_la_LDFLAGS = -module -avoid-version
#libnstbalsa_la_LIBADD = $(BALSA_LIBS)

libnstsylpheed_la_SOURCES = sylpheed-claws.c
libnstsylpheed_la_LDFLAGS = -module -avoid-version
#libnstsylpheed_la_LIBADD = $(SYLPHEED_LIBS)

libnstthunderbird_la_SOURCES = thunderbird.c
libnstthunderbird_la_LDFLAGS = -module -avoid-version
#libnstthunderbird_la_LIBADD = $(THUNDERBIRD_LIBS)

libnstgajim_la_SOURCES = gajim.c
libnstgajim_la_LDFLAGS = -module -avoid-version
libnstgajim_la_LIBADD = $(DBUS_LIBS)

EXTRA_DIST = bluetooth-marshal.list
CLEANFILES = bluetooth-marshal.h bluetooth-marshal.c

