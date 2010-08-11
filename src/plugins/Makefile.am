SUBDIRS = . removable-devices evolution
#SUBDIRS = . $(PLUGINS)
DIST_SUBDIRS = $(ALL_PLUGINS)

NST_COMMON_SOURCES = nst-common.c nst-common.h

noinst_LTLIBRARIES = libnstcommon.la
libnstcommon_la_SOURCES = $(NST_COMMON_SOURCES)
libnstcommon_la_CFLAGS = $(NAUTILUS_SENDTO_CFLAGS)
libnstcommon_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)

EXTRA_DIST = $(NST_COMMON_SOURCES)

nautilus_sendto_includedir = $(includedir)/nautilus-sendto/
nautilus_sendto_include_HEADERS = nautilus-sendto-plugin.h

lib_LTLIBRARIES = libnautilussendto-1.0.la
libnautilussendto_1_0_la_SOURCES = nautilus-sendto-plugin.h nautilus-sendto-plugin.c
libnautilussendto_1_0_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)
libnautilussendto_1_0_la_CFLAGS = $(NAUTILUS_SENDTO_CFLAGS)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = NautilusSendto-1.0.gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(top_builddir)/src --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)/libpeas

introspection_sources = $(libnautilussendto_1_0_la_SOURCES)

NautilusSendto-1.0.gir: libnautilussendto-1.0.la
NautilusSendto_1_0_gir_INCLUDES = GObject-2.0 Gtk-3.0 Peas-1.0
NautilusSendto_1_0_gir_CFLAGS = $(NAUTILUS_SENDTO_CFLAGS) -I$(top_srcdir)
NautilusSendto_1_0_gir_LIBS = libnautilussendto-1.0.la
NautilusSendto_1_0_gir_FILES = $(addprefix $(srcdir)/,$(introspection_sources))

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES = $(gir_DATA) $(typelib_DATA)
endif

