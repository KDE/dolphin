
INCLUDES= @all_includes@ 

CLEANFILES = $(BUILT_SOURCES)
bin_PROGRAMS = kfind
LDADD += @all_libraries@ -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 @LIBSOCKET@ 
MOC 	= @MOC@
ICONSDIR=$(prefix)/share/icons

####### Files

kfind_SOURCES  =  kfarch.cpp kfdird.cpp kfindtop.cpp  kftabdlg.cpp \
	     kfsave.cpp kftypes.cpp kfoptions.cpp kfwin.cpp kfind.cpp \
	     main.cpp $(BUILT_SOURCES)
BUILT_SOURCES  =  mkfdird.cpp mkfwin.cpp mkfindtop.cpp \
             mkftabdlg.cpp mkfoptions.cpp mkfind.cpp
noinst_HEADERS	  =  kfarch.h kfdird.h kfindtop.h kftabdlg.h kfwin.h kfind.h \
             kfsave.h kftypes.h kfoptions.h

SUBDIRS = pics

install-data-local:
	$(mkinstalldirs) $(ICONSDIR)
	$(INSTALL_DATA) kfind.xpm $(ICONSDIR)

uninstall-local:
	rm -f $(ICONSDIR)/$(TARGET).xpm

mkfdird.cpp: kfdird.h
	$(MOC) kfdird.h -o mkfdird.cpp

mkfwin.cpp: kfwin.h
	$(MOC) kfwin.h -o mkfwin.cpp

mkfindtop.cpp: kfindtop.h
	$(MOC) kfindtop.h -o mkfindtop.cpp

mkftabdlg.cpp: kftabdlg.h
	$(MOC) kftabdlg.h -o mkftabdlg.cpp

mkfoptions.cpp: kfoptions.h
	$(MOC) kfoptions.h -o mkfoptions.cpp

mkfind.cpp: kfind.h
	$(MOC) kfind.h -o mkfind.cpp

