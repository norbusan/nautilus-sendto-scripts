plugindir = $(libdir)/pidgin
plugin_LTLIBRARIES = nautilus.la

nautilus_la_SOURCES = nautilus-sendto-plugin.c
nautilus_la_LIBADD = $(PIDGIN_LIBS)
nautilus_la_LDFLAGS = -module -avoid-version -no-undefined

AM_CPPFLAGS = 					\
	-DLIBDIR=\"$(PIDGIN_LIBDIR)/pidgin/\" 	\
	-DDATADIR=\"$(PIDGIN_DATADIR)\" 	\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(PIDGIN_CFLAGS)			\
	$(WARN_CFLAGS)
