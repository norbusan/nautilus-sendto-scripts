SUBDIRS = plugins
INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
	-DPLUGINDIR=\"$(libdir)/nautilus-sendto/plugins\"	\
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
	-DGLADEDIR=\""$(gladedir)"\"		\
        $(NAUTILUS_SENDTO_CFLAGS)

glade_DATA = nautilus-sendto.glade
gladedir = $(datadir)/nautilus-sendto/glade

bin_PROGRAMS = nautilus-sendto

nautilus_sendto_SOURCES =			\
	nautilus-sendto-command.c		\
	nautilus-sendto-plugin.h

nautilus_sendto_LDADD = $(NAUTILUS_SENDTO_LIBS)

nautilus_extensiondir=$(libdir)/nautilus/extensions-1.0
nautilus_extension_LTLIBRARIES=libnautilus-sendto.la
libnautilus_sendto_la_SOURCES =		\
	nautilus-nste.c			\
	nautilus-nste.h			\
	nautilus-sendto-module.c

libnautilus_sendto_la_LDFLAGS = -module -avoid-version
libnautilus_sendto_la_LIBADD  = $(NAUTILUS_SENDTO_LIBS)

EXTRA_DIST = $(glade_DATA)
