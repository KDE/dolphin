
INCLUDES= -I.. $(all_includes) -I$(idldir)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD   = -lkparts -lkpartsui -lkom $(LIB_KIO) $(LIBMICO)

IDL_FLAGS = --query-server-for-narrow --relative-paths -I.. -I$(idldir)

konqueror.cc: konqueror.h
konqueror.h: ../konqueror.idl
	$(IDL) $(IDL_FLAGS) ../konqueror.idl

bin_PROGRAMS = kfmclient

kfmclient_SOURCES = kfmclient.cc konqueror.cc
noinst_HEADERS = kfmclient.h

BUILT_SOURCES =

####### Meta objects
