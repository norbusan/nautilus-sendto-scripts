SUBDIRS = plugins
INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
	-DPLUGINDIR=\"$(libdir)/nautilus-sendto/plugins\"	\
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
	-DGLADEDIR=\""$(gladedir)"\"		\
        $(NAUTILUS_SENDTO_CFLAGS)

glade_DATA =					\
	nautilus-sendto.glade			\
	nautilus-sendto-force.glade
gladedir = $(datadir)/nautilus-sendto/glade

bonobodir = $(libdir)/bonobo
bonobo_LTLIBRARIES = libnautilus-sendto-component.la

libnautilus_sendto_component_la_SOURCES =       \
	nautilus-sendto-component.c		\
	nautilus-sendto-component.h		\
	libmain.c

libnautilus_sendto_component_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)
libnautilus_sendto_component_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = nautilus-sendto

nautilus_sendto_SOURCES =			\
	nautilus-sendto-command.c		\
	nautilus-sendto-plugin.h

nautilus_sendto_LDADD = $(NAUTILUS_SENDTO_LIBS)

server_in_files = Nautilus_Sendto_Component.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files)
	sed -e "s|\@BONOBODIR\@|$(bonobodir)|" $< > $@

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(server_in_files) $(glade_DATA)
CLEANFILES = $(server_DATA) $(server_DATA).in
