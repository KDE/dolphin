INCLUDES  =  -I$(top_srcdir)/konqueror $(all_includes)
LDFLAGS   = $(all_libraries) $(KDE_RPATH)
LDADD = -lkdeui

lib_LTLIBRARIES = libkonqsearcher.la

libkonqsearcher_la_LDFLAGS = -version-info 1:0:0

libkonqsearcher_la_SOURCES = main.cc configwidget.cc enginecfg.cc

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

rc_DATA = konq_searcherrc
rcdir = $(kde_confdir)

METASOURCES = AUTO
