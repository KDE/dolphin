
INCLUDES= @all_includes@ 

CLEANFILES = $(BUILT_SOURCES)
bin_PROGRAMS = kfind
LDADD += @all_libraries@ -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 @LIBSOCKET@ 
MOC 	= @MOC@
PICSDIR=$(libdir)/pics

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfindtop.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp $(BUILT_SOURCES)
BUILT_SOURCES  =  mkfdird.cpp mkfwin.cpp mkfindtop.cpp \
             mkftabdlg.cpp mkfoptions.cpp mkfind.cpp
noinst_HEADERS	  =  kfarch.h kfdird.h kfindtop.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h

install-data-local:
	$(mkinstalldirs) $(PICSDIR)
	$(INSTALL_DATA) kfind.xpm $(PICSDIR)
	$(mkinstalldirs) $(prefix)/apps/Utilities
	$(INSTALL_DATA) 'KDE Find Tool.kdelnk' $(prefix)/apps/Utilities
	(cd doc && $(MAKE) install)
	(cd pics && $(MAKE) install)

uninstall-local:
	rm -f $(PICSDIR)/kfind.xpm
	rm -f '$(prefix)/apps/Utilities/KDE Find Tool.kdelnk'
	(cd doc && $(MAKE) uninstall)
	(cd pics && $(MAKE) uninstall)

mkfdird.cpp: kfdird.h
	$(MOC) kfdird.h -o $@

mkfwin.cpp: kfwin.h
	$(MOC) kfwin.h -o $@

mkfindtop.cpp: kfindtop.h
	$(MOC) kfindtop.h -o $@

mkftabdlg.cpp: kftabdlg.h
	$(MOC) kftabdlg.h -o $@

mkfoptions.cpp: kfoptions.h
	$(MOC) kfoptions.h -o $@

mkfind.cpp: kfind.h
	$(MOC) kfind.h -o $@

