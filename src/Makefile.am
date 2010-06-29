SUBDIRS = plugins
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

nautilus_sendto_includedir = $(includedir)/nautilus-sendto/
nautilus_sendto_include_HEADERS = nautilus-sendto-plugin.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nautilus-sendto.pc

ui_DATA = nautilus-sendto.ui
uidir = $(datadir)/nautilus-sendto/ui

noinst_LTLIBRARIES = libnstplugin.la
libnstplugin_la_SOURCES =			\
	nautilus-sendto-plugin.h		\
	nst-enum-types.c			\
	nst-enum-types.h
libnstplugin_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)

bin_PROGRAMS = nautilus-sendto

nautilus_sendto_SOURCES = nautilus-sendto-command.c
nautilus_sendto_LDADD = $(NAUTILUS_SENDTO_LIBS) libnstplugin.la

nautilus_extensiondir = $(NAUTILUS_EXTENSION_DIR)
nautilus_extension_LTLIBRARIES = libnautilus-sendto.la
libnautilus_sendto_la_SOURCES =		\
	nautilus-nste.c			\
	nautilus-nste.h			\
	nautilus-sendto-module.c

libnautilus_sendto_la_LDFLAGS = -module -avoid-version -no-undefined
libnautilus_sendto_la_LIBADD  = $(NAUTILUS_EXT_SENDTO_LIBS)

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
			--fhead "#include <nautilus-sendto-plugin.h>\n" \
			--fhead "#include \"nst-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(nst_headers_to_scan_for_enums)) > $@

convertdir=$(datadir)/GConf/gsettings
convert_DATA = nautilus-sendto-convert

gsettingsschema_in_files = org.gnome.Nautilus.Sendto.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = $(ui_DATA) nautilus-sendto.pc.in $(convert_DATA) $(gsettingsschema_in_files)
CLEANFILES = $(nst_built_headers) $(nst_built_cfiles)
