kde_module_LTLIBRARIES = kcm_performance.la

kcm_performance_la_SOURCES = kcmperformance.cpp konqueror.cpp konqueror_ui.ui
kcm_performance_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_performance_la_LIBADD = -lkdeui
INCLUDES= $(all_includes) 

kcm_performance_la_METASOURCES = AUTO

noinst_HEADERS = konqueror.h

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmperformance.pot

data_DATA = kcmperformance.desktop kcmkonqyperformance.desktop
datadir = $(kde_appsdir)/Settings/Components/

EXTRA_DIST = $(data_DATA)
