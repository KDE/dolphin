
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes) $(MICO_INCLUDES) 
LDFLAGS = $(KDE_RPATH) -L$(top_builddir)/libkonq $(all_libraries)
LDADD   = -lkparts $(LIB_KIO) -lkonq -lkded

# this gives better compile-time warnings.
AM_CXXFLAGS += -pedantic

IDL_FLAGS = --query-server-for-narrow --relative-paths --any -I$(srcdir)/.. -I$(idldir)

DISTCLEANFILES = browser.cc browser.h konqueror.cc konqueror.h kdesktop.cc kdesktop.h

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/../konqueror.idl $(micodir)/include/CORBA.h
	$(IDL) $(IDL_FLAGS) $(srcdir)/../konqueror.idl

kdesktop.cc: kdesktop.h
kdesktop.h: $(top_srcdir)/kdesktop/kdesktop.idl $(micodir)/include/CORBA.h
	$(IDL) $(IDL_FLAGS) $(top_srcdir)/kdesktop/kdesktop.idl

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = browser.idl kfmclient.cc konqueror.cc kdesktop.cc
noinst_HEADERS = kfmclient.h

browser_IDLDIR = $(idldir)

konqueror.h: browser.h
kfmclient.h : konqueror.h kdesktop.h
kfmclient.cc : konqueror.h kdesktop.h

