
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes) $(MICO_INCLUDES) 
LDFLAGS = $(KDE_RPATH) -L$(top_builddir)/libkonq $(all_libraries)
LDADD   = -lkparts $(LIB_KIO) -lkonq -lkded

# this gives better compile-time warnings.
AM_CXXFLAGS += -pedantic

IDL_FLAGS = --query-server-for-narrow --relative-paths --any --no-poa --no-boa -I$(srcdir)/.. -I$(idldir)

DISTCLEANFILES = browser.cc browser.h konqueror.cc konqueror.h kdesktop.cc kdesktop.h

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = browser.idl konqueror.idl kdesktop.idl kfmclient.cc
noinst_HEADERS = kfmclient.h

browser_IDLDIR = $(idldir)
konqueror_IDLDIR = $(top_srcdir)/konqueror
kdesktop_IDLDIR = $(top_srcdir)/kdesktop

