## Makefile.am of kdebase/konqueror/plugins/searcher

INCLUDES  =  -I$(top_srcdir)/kcontrol/ebrowsing/kurifilter -I$(top_srcdir)/konqueror -I$(top_srcdir)/lib/kparts $(all_includes)

lib_LTLIBRARIES = libkonqsearcher.la

libkonqsearcher_la_LDFLAGS = -version-info 1:0:0 -module -no-undefined $(all_libraries)
libkonqsearcher_la_LIBADD = ../../libkonqueror.la $(top_builddir)/kcontrol/ebrowsing/kurifilter/libkurifilter.la 

libkonqsearcher_la_SOURCES = konq_searcher.cc

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.h *.cc -o $(podir)/konq_searcher.pot

