
INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD   = $(LIB_KIO) 

####### Files

bin_PROGRAMS = kfmexec

kfmexec_SOURCES = main.cpp
noinst_HEADERS = main.h
METASOURCES = AUTO

messages:
	$(XGETTEXT) $(kfmexec_SOURCES) -o $(podir)/kfmexec.pot
