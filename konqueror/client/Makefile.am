
INCLUDES= -I.. $(all_includes) $(MICO_INCLUDES) -I$(idldir)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD   = -lkparts 

# this gives better compile-time warnings.
CXXFLAGS += -pedantic

IDL_FLAGS = --query-server-for-narrow --relative-paths -I$(srcdir)/.. -I$(idldir)

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/../konqueror.idl
	$(IDL) $(IDL_FLAGS) $(srcdir)/../konqueror.idl

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc konqueror.cc
noinst_HEADERS = kfmclient.h

BUILT_SOURCES = konqueror.h

