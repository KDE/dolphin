
INCLUDES= $(all_includes)
LDADD   = $(LIB_KIO) 

####### Files

bin_PROGRAMS = kfmexec

kfmexec_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kfmexec_SOURCES = main.cpp
noinst_HEADERS = main.h
METASOURCES = AUTO

messages:
	$(XGETTEXT) $(kfmexec_SOURCES) -o $(podir)/kfmexec.pot
