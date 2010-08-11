INCLUDES =					\
	-DDATADIR=\"$(datadir)\"		\
	-DPLUGINDIR=\"$(libdir)/nautilus-sendto/plugins\"	\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-DUIDIR=\""$(uidir)"\"		\
	-DLOCALEDIR="\"$(datadir)/locale\""	\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(NAUTILUS_EXT_SENDTO_CFLAGS)		\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

nautilus_extensiondir = $(NAUTILUS_EXTENSION_DIR)
nautilus_extension_LTLIBRARIES = libnautilus-sendto.la
libnautilus_sendto_la_SOURCES =		\
	nautilus-nste.c			\
	nautilus-nste.h			\
	nautilus-sendto-module.c

libnautilus_sendto_la_LDFLAGS = -module -avoid-version -no-undefined
libnautilus_sendto_la_LIBADD  = $(NAUTILUS_EXT_SENDTO_LIBS)

