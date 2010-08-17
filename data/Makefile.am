pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nautilus-sendto.pc

ui_DATA = nautilus-sendto.ui pack-entry.ui
uidir = $(datadir)/nautilus-sendto/ui

convertdir=$(datadir)/GConf/gsettings
convert_DATA = nautilus-sendto-convert

gsettingsschema_in_files = org.gnome.Nautilus.Sendto.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = $(ui_DATA) nautilus-sendto.pc.in $(convert_DATA) $(gsettingsschema_in_files)
