INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

# should be called libkonqhistorypart.la
kde_module_LTLIBRARIES = libkonqhistory.la

METASOURCES = AUTO

libkonqhistory_la_SOURCES = konq_history.cc historywidget.cc
libkonqhistory_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqhistory_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

servicedir = $(kde_servicesdir)
service_DATA = konq_history.desktop
