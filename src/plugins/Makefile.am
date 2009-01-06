plugindir = $(libdir)/nautilus-sendto/plugins

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)                         \
        -I$(top_builddir)                       \
        -DGLADEDIR=\""$(gladedir)"\"            \
        $(NAUTILUS_SENDTO_CFLAGS)               \
	$(NST_EBOOK_CFLAGS)			\
	$(GAIM_CFLAGS)				\
	$(PIDGIN_CFLAGS)			\
	$(BLUETOOTH_CFLAGS)			\
	$(UPNP_CFLAGS)				\
	$(DBUS_CFLAGS)				\
	$(WARN_CFLAGS)				\
	$(EMPATHY_CFLAGS)

if HAVE_GAIM
GAIM_PLUGIN = libnstgaim.la
else
GAIM_PLUGIN =
endif

if HAVE_PIDGIN
PIDGIN_PLUGIN = libnstpidgin.la
else
PIDGIN_PLUGIN =
endif

if HAVE_BLUETOOTH
BLUETOOTH_PLUGIN = libnstbluetooth.la
else
BLUETOOTH_PLUGIN = 
endif

if HAVE_UPNP
UPNP_PLUGIN = libnstupnp.la
else
UPNP_PLUGIN =
endif

if HAVE_EVOLUTION
EVOLUTION_PLUGIN = libnstevolution.la
else
EVOLUTION_PLUGIN =
endif

if HAVE_BALSA
BALSA_PLUGIN = libnstbalsa.la
else
BALSA_PLUGIN = 
endif

if HAVE_SYLPHEED
SYLPHEED_PLUGIN = libnstsylpheed.la
else
SYLPHEED_PLUGIN = 
endif

if HAVE_THUNDERBIRD
THUNDERBIRD_PLUGIN = libnstthunderbird.la
else
THUNDERBIRD_PLUGIN = 
endif

if HAVE_GAJIM
GAJIM_PLUGIN = libnstgajim.la
else
GAJIM_PLUGIN = 
endif

if HAVE_EMPATHY
EMPATHY_PLUGIN = libnstempathy.la
else
EMPATHY_PLUGIN  =
endif

MARSHALFILES = econtactentry-marshal.c econtactentry-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

econtactentry-marshal.c: econtactentry-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header --body > econtactentry-marshal.c )
econtactentry-marshal.h: econtactentry-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=econtactentry_marshal $(srcdir)/econtactentry-marshal.list --header > econtactentry-marshal.h )

CLA_FILES = e-contact-entry.h e-contact-entry.c econtactentry-marshal.list
CLADIR = $(srcdir)/../../../contact-lookup-applet/src/
regenerate-built-sources:
	EGGFILES="$(CLA_FILES) econtactentry-marshal.list" EGGDIR="$(CLADIR)" $(srcdir)/update-from-egg.sh || true

plugin_LTLIBRARIES = $(EVOLUTION_PLUGIN) $(GAIM_PLUGIN) $(PIDGIN_PLUGIN) $(BLUETOOTH_PLUGIN) $(UPNP_PLUGIN) $(BALSA_PLUGIN) $(SYLPHEED_PLUGIN) $(THUNDERBIRD_PLUGIN) $(GAJIM_PLUGIN) $(EMPATHY_PLUGIN)

libnstevolution_la_SOURCES = evolution.c $(CLA_FILES) $(MARSHALFILES)
libnstevolution_la_LDFLAGS = -module -avoid-version
libnstevolution_la_LIBADD = $(NST_EBOOK_LIBS)

libnstgaim_la_SOURCES = gaim.c
libnstgaim_la_LDFLAGS = -module -avoid-version
libnstgaim_la_LIBADD = $(GAIM_LIBS)

libnstpidgin_la_SOURCES = pidgin.c
libnstpidgin_la_LDFLAGS = -module -avoid-version
libnstpidgin_la_LIBADD = $(PIDGIN_LIBS)

bluetooth-marshal.h: bluetooth-marshal.list
	$(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --header > $@

bluetooth-marshal.c: bluetooth-marshal.list
	$(GLIB_GENMARSHAL) --prefix=nst_bluetooth_marshal $< --body > $@
bluetooth.c: bluetooth-marshal.c bluetooth-marshal.h

libnstbluetooth_la_SOURCES = bluetooth.c bluetooth-marshal.h bluetooth-marshal.c
libnstbluetooth_la_LDFLAGS = -module -avoid-version
libnstbluetooth_la_LIBADD = $(BLUETOOTH_LIBS)

libnstupnp_la_SOURCES = upnp.c
libnstupnp_la_LDFLAGS = -module -avoid-version
libnstupnp_la_LIBADD = $(UPNP_LIBS)

libnstbalsa_la_SOURCES = balsa.c
libnstbalsa_la_LDFLAGS = -module -avoid-version
#libnstbalsa_la_LIBADD = $(BALSA_LIBS)

libnstsylpheed_la_SOURCES = sylpheed-claws.c
libnstsylpheed_la_LDFLAGS = -module -avoid-version
#libnstsylpheed_la_LIBADD = $(SYLPHEED_LIBS)

libnstthunderbird_la_SOURCES = thunderbird.c
libnstthunderbird_la_LDFLAGS = -module -avoid-version
#libnstthunderbird_la_LIBADD = $(THUNDERBIRD_LIBS)

libnstgajim_la_SOURCES = gajim.c
libnstgajim_la_LDFLAGS = -module -avoid-version
libnstgajim_la_LIBADD = $(DBUS_LIBS)

libnstempathy_la_SOURCES = empathy.c
libnstempathy_la_LDFLAGS = -module -avoid-version
libnstempathy_la_LIBADD = $(EMPATHY_LIBS)

EXTRA_DIST = bluetooth-marshal.list $(CLA_FILES)
CLEANFILES = bluetooth-marshal.h bluetooth-marshal.c econtactentry-marshal.c econtactentry-marshal.h

