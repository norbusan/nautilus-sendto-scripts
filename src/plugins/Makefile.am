plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)

if HAVE_GAIM
GAIM_PLUGIN = libgaim.la
else
GAIM_PLUGIN =
endif

plugin_LTLIBRARIES = libevolution.la $(GAIM_PLUGIN) 

libevolution_la_SOURCES = evolution.c
libevolution_la_LDFLAGS = -module -avoid-version

libgaim_la_SOURCES = gaim.c
libgaim_la_LDFLAGS = -module -avoid-version

