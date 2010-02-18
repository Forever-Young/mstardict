imagesdir = $(datadir)/icons/hicolor/48x48/hildon
images_DATA = mstardict.png

desktopdir = $(HILDON_DESKTOP_ENTRY_DIR)
desktop_in_files = mstardict.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES = $(desktop_DATA)
