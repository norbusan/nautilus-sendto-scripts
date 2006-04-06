plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(NST_EBOOK_CFLAGS)			\
	$(GAIM_CFLAGS)				\
	$(BLUETOOTH_CFLAGS)

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

plugin_LTLIBRARIES = $(EVOLUTION_PLUGIN) $(GAIM_PLUGIN) $(BLUETOOTH_PLUGIN)

libnstevolution_la_SOURCES = evolution.c
libnstevolution_la_LDFLAGS = -module -avoid-version
libnstevolution_la_LIBADD = $(NST_EBOOK_LIBS)

libnstgaim_la_SOURCES = gaim.c
libnstgaim_la_LDFLAGS = -module -avoid-version
libnstgaim_la_LIBADD = $(GAIM_LIBS)

libnstbluetooth_la_SOURCES = bluetooth.c
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

