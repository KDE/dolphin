
INCLUDES= $(all_includes) 

bin_PROGRAMS = kfind
LDFLAGS =  $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KFILE)

ICONSDIR=$(kde_icondir)

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfindtop.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp moc.cpp

METASOURCES  =  kfdird.moc kfwin.moc kfindtop.moc \
             kftabdlg.moc kfoptions.moc kfind.moc

noinst_HEADERS	  =  kfarch.h kfdird.h kfindtop.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h version.h

EXTRA_DIST = kfind.xpm mini-kfind.xpm

SUBDIRS = pics

install-data-local:
	$(mkinstalldirs) $(ICONSDIR)
	$(INSTALL_DATA) $(srcdir)/kfind.xpm $(ICONSDIR)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/mini-kfind.xpm $(kde_minidir)/kfind.xpm

uninstall-local:
	rm -f $(ICONSDIR)/$(TARGET).xpm
	rm -f $(kde_minidir)/kfind.xpm

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kfind_SOURCES) && mv messages.po ../po/kfind.pot

