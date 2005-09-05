## Makefile.am of kdebase/kcontrol/ebrowsing/plugins/ikws

SUBDIRS = searchproviders

AM_CPPFLAGS   = $(all_includes)

METASOURCES = AUTO

kde_module_LTLIBRARIES = libkuriikwsfilter.la libkurisearchfilter.la

libkuriikwsfilter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libkuriikwsfilter_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI)

libkurisearchfilter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libkurisearchfilter_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI)

libkuriikwsfilter_la_SOURCES = kuriikwsfiltereng.cpp kuriikwsfilter.skel \
    kuriikwsfilter.cpp searchprovider.cpp

libkurisearchfilter_la_SOURCES = kurisearchfilter.cpp kurisearchfilter.skel \
      kuriikwsfiltereng.cpp ikwsopts.cpp searchproviderdlg.cpp searchprovider.cpp \
      searchproviderdlg_ui.ui ikwsopts_ui.ui

update_DATA = kuriikwsfilter.upd
updatedir = $(kde_datadir)/kconf_update

service_DATA = kuriikwsfilter.desktop kurisearchfilter.desktop
servicedir = $(kde_servicesdir)

servicetypes_DATA = searchprovider.desktop
servicetypesdir = $(kde_servicetypesdir)
