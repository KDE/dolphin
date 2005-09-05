AM_CPPFLAGS = $(all_includes)

template_DATA = template.css
templatedir = $(kde_datadir)/kcmcss

kde_module_LTLIBRARIES = kcm_css.la
kcm_css_la_SOURCES = template.cpp cssconfig.ui csscustom.ui kcmcss.cpp preview.ui
kcm_css_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_css_la_LIBADD = -lkdeui $(LIB_KIO)

METASOURCES = AUTO
 
messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmcss.pot
 
xdg_apps_DATA = kcmcss.desktop
