SUBDIRS = init

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqdirtree.la

METASOURCES = AUTO

libkonqdirtree_la_SOURCES = konq_dirtree.cc
libkonqdirtree_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkonqdirtree_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

servicedir = $(kde_servicesdir)
service_DATA = konq_dirtree.desktop
