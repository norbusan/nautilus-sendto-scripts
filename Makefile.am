if HAVE_GAIM
GAIM_PLUGIN_DIR=gaim_plugin
endif

SUBDIRS = src $(GAIM_PLUGIN_DIR) po

EXTRA_DIST =                    \
        AUTHORS                 \
	NEWS                    \
	README                  \
	intltool-merge.in       \
	intltool-update.in      \
	intltool-extract.in	\
	$(schema_in_files)

CLEANFILES = 			\
	$(schema_DATA)

DISTCLEANFILES =                        \
	po/.intltool-merge-cache        \
	intltool-extract                \
	intltool-merge                  \
	intltool-update

schema_in_files = nst.schemas.in
schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)


