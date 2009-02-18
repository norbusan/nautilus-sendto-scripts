%.eplug: %.eplug.in
	sed -e 's|\@PLUGINDIR\@|$(plugindir)|'		\
	-e 's|\@SOEXT\@|$(SOEXT)|'			\
	-e 's|\@GETTEXT_PACKAGE\@|$(GETTEXT_PACKAGE)|'	\
	-e 's|\@LOCALEDIR\@|$(localedir)|' $< > $@

%.eplug.in: %.eplug.xml
	LC_ALL=C $(INTLTOOL_MERGE) -x -u /tmp $< $@

%.error: %.error.xml
	LC_ALL=C $(INTLTOOL_MERGE) -x -u /tmp $< $@
