
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS = $(KDE_RPATH) -L$(top_builddir)/libkonq $(all_libraries)
LDADD   = $(LIB_KIO) -lkonq

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc KonquerorIface.stub KDesktopIface.stub
noinst_HEADERS = kfmclient.h
METASOURCES = AUTO

KonquerorIface_DIR =  $(srcdir)/..
KDesktopIface_DIR = $(top_srcdir)/kdesktop

messages:
	$(XGETTEXT) -kaliasLocal *.h *.cc -o $(podir)/kfmclient.pot

