SUBDIRS = . $(PLUGINS)
DIST_SUBDIRS = $(ALL_PLUGINS)

noinst_PROGRAMS = test-progress
test_progress_SOURCES = test-progress.c
test_progress_CFLAGS = $(NAUTILUS_SENDTO_CFLAGS)
test_progress_LDADD = $(NAUTILUS_SENDTO_LIBS) libnautilussendto-1.0.la

nautilus_sendto_includedir = $(includedir)/nautilus-sendto/
nautilus_sendto_include_HEADERS = nautilus-sendto-plugin.h nst-common.h nautilus-sendto-packer.h nautilus-sendto-filelist.h nautilus-sendto-progress.h

MARSHALFILES = nst-plugin-marshal.c nst-plugin-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

nst-plugin-marshal.c: nst-plugin-marshal.h
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=nst_plugin_marshal $(srcdir)/nst-plugin-marshal.list --header --body > nst-plugin-marshal.c )
nst-plugin-marshal.h: nst-plugin-marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=nst_plugin_marshal $(srcdir)/nst-plugin-marshal.list --header > nst-plugin-marshal.h )

lib_LTLIBRARIES = libnautilussendto-1.0.la
libnautilussendto_1_0_la_SOURCES =				\
	nautilus-sendto-plugin.c				\
	nst-common.c						\
	nautilus-sendto-packer.c				\
	nautilus-sendto-filelist.c				\
	nautilus-sendto-progress.c				\
	nst-enum-types.h					\
	nst-enum-types.c					\
	$(nautilus_sendto_include_HEADERS)			\
	$(MARSHALFILES)
libnautilussendto_1_0_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)
libnautilussendto_1_0_la_CFLAGS = $(NAUTILUS_SENDTO_CFLAGS) -DUIDIR=\""$(uidir)"\"

uidir = $(datadir)/nautilus-sendto/ui

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

nst_headers_to_scan_for_enums = nautilus-sendto-plugin.h
# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
nst_built_headers = nst-enum-types.h
nst_built_cfiles = nst-enum-types.c

nst-enum-types.h: $(nst_headers_to_scan_for_enums) Makefile
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __NST_ENUM_TYPES_H__\n#define __NST_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define NST_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __NST_ENUM_TYPES_H__ */" \
		$(nst_headers_to_scan_for_enums)) > $@

nst-enum-types.c: $(nst_headers_to_scan_for_enums) Makefile nst-enum-types.h
	$(AM_V_GEN) (cd $(srcdir) && glib-mkenums \
			--fhead "#include \"nautilus-sendto-plugin.h\"\n" \
			--fhead "#include \"nst-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(nst_headers_to_scan_for_enums)) > $@


CLEANFILES = $(gir_DATA) $(typelib_DATA) $(nst_built_headers) $(nst_built_cfiles)

