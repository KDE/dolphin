
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS = $(KDE_RPATH) -L$(top_builddir)/libkonq $(all_libraries)
LDADD   = $(LIB_KIO) -lkonq

# this gives better compile-time warnings.
AM_CXXFLAGS += -pedantic

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc KonquerorIface_stub.cpp
noinst_HEADERS = kfmclient.h

KonquerorIface.kidl: $(top_srcdir)/konqueror/KonquerorIface.h
	( kidl $(top_srcdir)/konqueror/KonquerorIface.h > KonquerorIface.kidl ; kidl2cpp --no-skel KonquerorIface.kidl )
KonquerorIface_stub.cpp: KonquerorIface.kidl

kfmclient.cc: KonquerorIface_stub.cpp
