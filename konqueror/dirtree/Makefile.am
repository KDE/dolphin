
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/konqueror $(all_includes)

lib_LTLIBRARIES = libkonqdirtree.la

METASOURCES = AUTO

libkonqdirtree_la_SOURCES = konq_dirtree.cc
libkonqdirtree_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module
libkonqdirtree_la_LIBADD = $(top_builddir)/konqueror/libkonqueror.la

servicedir = $(kde_servicesdir)
service_DATA = konq_dirtree.desktop

dirtreestuffdir = $(kde_datadir)/konqueror/dirtree
dirtreestuff_DATA = root.desktop home.desktop

dirtreeftpstuffdir = $(kde_datadir)/konqueror/dirtree/FTPs
dirtreeftpstuff_DATA = .directory kde_ftp.desktop


