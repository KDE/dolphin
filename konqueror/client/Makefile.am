
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS = $(KDE_RPATH) -L$(top_builddir)/libkonq $(all_libraries)
LDADD   = $(LIB_KIO) -lkonq

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc KonquerorIface_stub.cpp KDesktopIface_stub.cpp
noinst_HEADERS = kfmclient.h

KonquerorIface.kidl: $(srcdir)/../KonquerorIface.h
	kidl $(srcdir)/../KonquerorIface.h > KonquerorIface.kidl 

KonquerorIface_stub.cpp: KonquerorIface.kidl
	kidl2cpp --no-skel KonquerorIface.kidl

KDesktopIface.kidl: $(top_srcdir)/kdesktop/KDesktopIface.h
	kidl $(top_srcdir)/kdesktop/KDesktopIface.h > KDesktopIface.kidl 
	
KDesktopIface_stub.cpp: KDesktopIface.kidl
	kidl2cpp --no-skel KDesktopIface.kidl

kfmclient.cc: KonquerorIface_stub.cpp KDesktopIface_stub.cpp

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocal -ki18n -x $(includedir)/kde.pot *.h *.cc && mv messages.po ../../po/kfmclient.pot 

