plugindir = $(libdir)/gaim

nautilus_la_LDFLAGS = -module -avoid-version


if PLUGINS

plugin_LTLIBRARIES = nautilus.la

nautilus_la_SOURCES = nautilus-sendto-plugin.c
nautilus_la_LIBADD = $(NAUTILUS_SENDTO_CFLAGS)

endif


AM_CPPFLAGS = 					\
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" 	\
	-DDATADIR=\"$(GAIM_DATADIR)\" 		\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(GAIM_CFLAGS)
