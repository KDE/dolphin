INCLUDES = -I$(top_srcdir)/libltdl -I.. $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = testnsplugin.h

check_PROGRAMS = testnsplugin
testnsplugin_SOURCES = testnsplugin.cpp
testnsplugin_LDFLAGS = $(all_libraries) $(KDE_RPATH)
testnsplugin_LDADD = $(LIB_KDEUI) -L.. -lnsplugin
