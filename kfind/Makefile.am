
INCLUDES= @all_includes@ 

CLEANFILES = $(BUILT_SOURCES)
bin_PROGRAMS = kfind
LDADD += @all_libraries@ -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 @LIBSOCKET@ 
MOC 	= @MOC@

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfmenu.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp $(BUILT_SOURCES)
BUILT_SOURCES  =  mkfdird.cpp mkfwin.cpp mkfmenu.cpp \
             mkftabdlg.cpp mkfoptions.cpp mkfind.cpp
noinst_HEADERS	  =  kfarch.h kfdird.h kfmenu.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h

install-local-data:
	$(mkinstalldirs) $(PICSDIR)
	$(INSTALL_DATA) kfind.xpm $(PICSDIR)
	$(mkinstalldirs) $(prefix)/apps/Utilities
	$(INSTALL_DATA) kfind.kdelnk $(prefix)/apps/Utilities
	$(mkinstalldirs) $(LIBDIR)/kfind/config
	$(INSTALL_DATA) ./kfindrc $(prefix)/lib/kfind/config
	(cd doc && $(MAKE) install)

uninstall-local:
	rm -f $(PICSDIR)/$(TARGET).xpm
	rm -f $(prefix)/apps/Utility/kfind.kdelnk
	rm -f $(LIBDIR)/kfind/config/kfindrc
	(cd doc && $(MAKE) uninstall)

mkfdird.cpp: kfdird.h
	$(MOC) kfdird.h -o $@

mkfwin.cpp: kfwin.h
	$(MOC) kfwin.h -o $@

mkfmenu.cpp: kfmenu.h
	$(MOC) kfmenu.h -o $@

mkftabdlg.cpp: kftabdlg.h
	$(MOC) kftabdlg.h -o $@

mkfoptions.cpp: kfoptions.h
	$(MOC) kfoptions.h -o $@

mkfind.cpp: kfind.h
	$(MOC) kfind.h -o $@

