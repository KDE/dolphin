
INCLUDES= -I.. $(all_includes) $(MICO_INCLUDES) -I$(idldir)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD   = -lkparts $(LIB_KIO)

# this gives better compile-time warnings.
AM_CXXFLAGS += -pedantic

IDL_FLAGS = --query-server-for-narrow --relative-paths -I$(srcdir)/.. -I$(idldir)

DISTCLEANFILES = konqueror.cc konqueror.h

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/../konqueror.idl
	$(IDL) $(IDL_FLAGS) $(srcdir)/../konqueror.idl

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc konqueror.cc
noinst_HEADERS = kfmclient.h

BUILT_SOURCES = konqueror.h

