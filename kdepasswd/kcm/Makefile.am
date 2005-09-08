SUBDIRS = pics
kde_module_LTLIBRARIES = kcm_useraccount.la
kcm_useraccount_la_SOURCES = main_widget.ui chfnprocess.cpp \
			     main.cpp chfacedlg.cpp settings.kcfgc pass.kcfgc
kcm_useraccount_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_useraccount_la_LIBADD = $(top_builddir)/libkonq/libkonq.la

AM_CPPFLAGS = -I$(top_srcdir)/libkonq $(all_includes)

kde_kcfg_DATA = kcm_useraccount.kcfg kcm_useraccount_pass.kcfg

METASOURCES = AUTO

noinst_HEADERS = main.h chfnprocess.h chfacedlg.h

xdg_apps_DATA = kcm_useraccount.desktop 


messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/useraccount.pot
KDE_ICON=AUTO
