
INCLUDES= $(all_includes) 

bin_PROGRAMS = kfind
LDADD = $(all_libraries) -lkfile -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 $(LIBSOCKET) $(KDE_RPATH)

ICONSDIR=$(kde_icondir)

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfindtop.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp moc.cpp

BUILT_SOURCES  =  kfdird.moc kfwin.moc kfindtop.moc \
             kftabdlg.moc kfoptions.moc kfind.moc

DISTCLEANFILES = $(BUILT_SOURCES)

noinst_HEADERS	  =  kfarch.h kfdird.h kfindtop.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h version.h

EXTRA_DIST = kfind.xpm mini-kfind.xpm

SUBDIRS = pics

install-data-local:
	$(mkinstalldirs) $(ICONSDIR)
	$(INSTALL_DATA) kfind.xpm $(ICONSDIR)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-kfind.xpm $(kde_minidir)/kfind.xpm

uninstall-local:
	rm -f $(ICONSDIR)/$(TARGET).xpm
	rm -f $(kde_minidir)/kfind.xpm

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kfind_SOURCES) && mv messages.po ../po/kfind.pot

kfdird.moc: kfdird.h
	$(MOC) kfdird.h -o kfdird.moc

kfwin.moc: kfwin.h
	$(MOC) kfwin.h -o kfwin.moc

kfindtop.moc: kfindtop.h
	$(MOC) kfindtop.h -o kfindtop.moc

kftabdlg.moc: kftabdlg.h
	$(MOC) kftabdlg.h -o kftabdlg.moc

kfoptions.moc: kfoptions.h
	$(MOC) kfoptions.h -o kfoptions.moc

kfind.moc: kfind.h
	$(MOC) kfind.h -o kfind.moc

moc.cpp: $(BUILT_SOURCES)

