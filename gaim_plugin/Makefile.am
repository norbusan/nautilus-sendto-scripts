plugindir = $(libdir)/gaim
plugin_LTLIBRARIES = nautilus.la

nautilus_la_SOURCES = nautilus-sendto-plugin.c
nautilus_la_LIBADD = $(NAUTILUS_SENDTO_CFLAGS)
nautilus_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = 					\
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" 	\
	-DDATADIR=\"$(GAIM_DATADIR)\" 		\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(GAIM_CFLAGS)
