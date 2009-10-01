if HAVE_EVOLUTION
EVOLUTION_PLUGIN_DIR=evolution
endif

SUBDIRS = src $(EVOLUTION_PLUGIN_DIR) docs po

DIST_SUBDIRS = evolution src po

man_MANS = nautilus-sendto.1

EXTRA_DIST =                    \
        AUTHORS                 \
	NEWS                    \
	README                  \
	ChangeLog		\
	ChangeLog.pre-1.1.4.1	\
	intltool-merge.in       \
	intltool-update.in      \
	intltool-extract.in	\
	$(man_MANS)             \
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

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --after="Fri Apr 17 16:36:52 2009" > $@; \
	fi

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)


