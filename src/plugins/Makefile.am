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
GAIM_PLUGIN = libgaim.la
else
GAIM_PLUGIN =
endif

if HAVE_BLUETOOTH
BLUETOOTH_PLUGIN = libbluetooth.la
else
BLUETOOTH_PLUGIN = 
endif

plugin_LTLIBRARIES = libevolution.la $(GAIM_PLUGIN) $(BLUETOOTH_PLUGIN)

libevolution_la_SOURCES = evolution.c
libevolution_la_LDFLAGS = -module -avoid-version
libevolution_la_LIBADD = $(NST_EBOOK_LIBS)

libgaim_la_SOURCES = gaim.c
libgaim_la_LDFLAGS = -module -avoid-version
libgaim_la_LIBADD = $(GAIM_LIBS)

libbluetooth_la_SOURCES = bluetooth.c
libbluetooth_la_LDFLAGS = -module -avoid-version
libbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

