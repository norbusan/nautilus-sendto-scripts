plugindir = $(libdir)/pidgin
plugin_LTLIBRARIES = nautilus.la

nautilus_la_SOURCES = nautilus-sendto-plugin.c
nautilus_la_LIBADD = $(NAUTILUS_SENDTO_CFLAGS)
nautilus_la_LDFLAGS = -module -avoid-version

AM_CPPFLAGS = 					\
	-DLIBDIR=\"$(PIDGIN_LIBDIR)/pidgin/\" 	\
	-DDATADIR=\"$(PIDGIN_DATADIR)\" 		\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(PIDGIN_CFLAGS)
