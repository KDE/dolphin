INCLUDES  =  -I$(top_srcdir)/konqueror -I$(top_srcdir)/lib/kparts \
$(all_includes)
LDFLAGS   = $(all_libraries)
LDADD = -lkdeui

lib_LTLIBRARIES = libkonqsearcher.la

libkonqsearcher_la_LDFLAGS = -version-info 1:0:0 -module

libkonqsearcher_la_SOURCES = main.cc configwidget.cc enginecfg.cc

service_DATA = konq_searcher.desktop
servicedir = $(kde_servicesdir)

rc_DATA = konq_searcherrc
rcdir = $(kde_confdir)

METASOURCES = AUTO
