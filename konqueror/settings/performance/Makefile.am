kde_module_LTLIBRARIES = kcm_performance.la

kcm_performance_la_SOURCES = kcmperformance.cpp konqueror.cpp konqueror_ui.ui system.cpp system_ui.ui
kcm_performance_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_performance_la_LIBADD = -lkdeui
AM_CPPFLAGS= $(all_includes) 

METASOURCES = AUTO

noinst_HEADERS = konqueror.h system.h

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmperformance.pot

xdg_apps_DATA = kcmperformance.desktop

kcmperformance_hidden_data_DATA = kcmkonqyperformance.desktop
kcmperformance_hidden_datadir = $(kde_appsdir)/.hidden

EXTRA_DIST = $(xdg_apps_DATA)
