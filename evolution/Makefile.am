INCLUDES =						\
	$(EVOLUTION_CFLAGS)				\
	$(WARN_CFLAGS)

@EVO_PLUGIN_RULE@

plugin_DATA = org-gnome-evolution-send-attachments-to.eplug
plugin_LTLIBRARIES = liborg-gnome-evolution-send-attachments-to.la

liborg_gnome_evolution_send_attachments_to_la_SOURCES = nautilus-sendto.c
liborg_gnome_evolution_send_attachments_to_la_LDFLAGS = -module -avoid-version $(NO_UNDEFINED)
liborg_gnome_evolution_send_attachments_to_la_LIBADD = $(EVOLUTION_LIBS)

EXTRA_DIST = org-gnome-evolution-send-attachments-to.eplug.xml

BUILT_SOURCES = $(plugin_DATA)
CLEANFILES = $(BUILT_SOURCES)
