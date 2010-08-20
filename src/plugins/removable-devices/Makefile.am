plugindir = $(libdir)/nautilus-sendto/plugins

plugin_in_files = removable-devices.nst-plugin.in
%.nst-plugin: %.nst-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache
plugin_DATA = removable-devices.nst-plugin

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

plugin_LTLIBRARIES = libnstremovable_devices.la

libnstremovable_devices_la_SOURCES = removable-devices.c
libnstremovable_devices_la_LDFLAGS = -module -avoid-version
libnstremovable_devices_la_LIBADD = $(GIO_LIBS) $(NAUTILUS_SENDTO_LIBS) $(builddir)/../libnautilussendto-1.0.la

EXTRA_DIST = $(plugin_in_files)
