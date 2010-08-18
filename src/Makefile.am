include $(top_srcdir)/Makefile.decl

SUBDIRS = extension plugins
INCLUDES =					\
	-DDATADIR=\"$(datadir)\"		\
	-DPLUGINDIR=\"$(pkglibdir)/plugins\"	\
	-DLIBDIR=\"$(pkglibdir)\"		\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/src/plugins		\
	-DUIDIR=\""$(uidir)"\"			\
	-DLOCALEDIR="\"$(datadir)/locale\""	\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(NAUTILUS_EXT_SENDTO_CFLAGS)		\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

noinst_LTLIBRARIES = libnstplugin.la
libnstplugin_la_SOURCES =			\
	nst-enum-types.c			\
	nst-enum-types.h			\
	nautilus-sendto-mimetype.c		\
	nautilus-sendto-mimetype.h
libnstplugin_la_LIBADD = $(NAUTILUS_SENDTO_LIBS)

bin_PROGRAMS = nautilus-sendto

nautilus_sendto_SOURCES = nautilus-sendto-command.c
nautilus_sendto_LDADD = $(NAUTILUS_SENDTO_LIBS) libnstplugin.la $(builddir)/plugins/libnautilussendto-1.0.la

pwd=`pwd`

noinst_PROGRAMS = $(TEST_PROGS)
TEST_PROGS += test-mimetype
test_mimetype_SOURCES = test-mimetype.c
test_mimetype_LDADD = $(NAUTILUS_SENDTO_LIBS) libnstplugin.la $(builddir)/plugins/libnautilussendto-1.0.la

nst_headers_to_scan_for_enums = plugins/nautilus-sendto-plugin.h
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
			--fhead "#include <plugins/nautilus-sendto-plugin.h>\n" \
			--fhead "#include \"nst-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(nst_headers_to_scan_for_enums)) > $@

CLEANFILES = $(nst_built_headers) $(nst_built_cfiles)
