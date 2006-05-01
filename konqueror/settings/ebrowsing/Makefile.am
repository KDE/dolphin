
SUBDIRS = plugins

AM_CPPFLAGS  = -I$(srcdir)/plugins/ikws $(all_includes)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_kurifilt.la
kcm_kurifilt_la_SOURCES = filteropts.cpp main.cpp

kcm_kurifilt_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_kurifilt_la_LIBADD = $(LIB_KSYCOCA)
# Don't link this lib to the plugins. They are dlopened and linking to them
# results in crashes when closing kcmshell.
# plugins/ikws/libkuriikwsfilter.la plugins/shorturi/libkshorturifilter.la


xdg_apps_DATA = ebrowsing.desktop
