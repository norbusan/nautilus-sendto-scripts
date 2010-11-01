include $(top_srcdir)/Makefile.decl

SUBDIRS = plugins
INCLUDES =					\
	-DDATADIR=\"$(datadir)\"		\
	-DPLUGINDIR=\"$(pkglibdir)/plugins\"	\
	-DLIBDIR=\"$(pkglibdir)\"		\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/src/plugins		\
	-DUIDIR=\""$(uidir)"\"			\
	-DLOCALEDIR="\"$(datadir)/locale\""	\
	-DTEST_SRCDIR=\""$(srcdir)/"\"		\
	$(NAUTILUS_SENDTO_CFLAGS)		\
	$(NAUTILUS_EXT_SENDTO_CFLAGS)		\
	$(DISABLE_DEPRECATED)			\
	$(WARN_CFLAGS)

noinst_LTLIBRARIES = libnstplugin.la
libnstplugin_la_SOURCES =			\
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

EXTRA_DIST = test-mimetype-data
