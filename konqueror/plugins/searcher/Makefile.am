## Makefile.am of kdebase/konqueror/plugins/searcher

INCLUDES  =  -I$(top_srcdir)/kcontrol/ebrowsing/kurifilter -I$(top_srcdir)/konqueror -I$(top_srcdir)/lib/kparts $(all_includes)
LDFLAGS   = $(KDE_RPATH) $(all_libraries)
LDADD = $(top_builddir)/kcontrol/ebrowsing/kurifilter/libkurifilter.la -lkdeui

lib_LTLIBRARIES = libkonqsearcher.la

libkonqsearcher_la_LDFLAGS = -version-info 1:0:0 -module
libkonqsearcher_la_LIBADD = $(top_builddir)/kcontrol/ebrowsing/kurifilter/libkurifilter.la -lkdeui

libkonqsearcher_la_SOURCES = konq_searcher.cc

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT)  -kaliasLocal *.h *.cc -o $(podir)/konq_searcher.pot
