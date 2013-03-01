plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        -I$(srcdir)/../				\
        $(NAUTILUS_SENDTO_CFLAGS)               \
        $(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libnstscripts.la

libnstscripts_la_SOURCES = scripts.c
libnstscripts_la_LDFLAGS = -module -avoid-version
libnstscripts_la_LIBADD = $(NAUTILUS_SENDTO_LIBS) $(builddir)/../libnstcommon.la

