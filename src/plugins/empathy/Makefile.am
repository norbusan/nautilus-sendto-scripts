plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/src                     \
        -I$(top_builddir)                       \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(WARN_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(EMPATHY_CFLAGS)

plugin_LTLIBRARIES = libnstempathy.la

libnstempathy_la_SOURCES = empathy.c
libnstempathy_la_LDFLAGS = -module -avoid-version
libnstempathy_la_LIBADD = $(EMPATHY_LIBS)

