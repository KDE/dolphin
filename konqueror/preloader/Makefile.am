AM_CPPFLAGS = -DQT_NO_CAST_ASCII

kde_module_LTLIBRARIES = kded_konqy_preloader.la

INCLUDES= -I.. $(all_includes)
kded_konqy_preloader_la_SOURCES = preloader.cc preloader.skel
kded_konqy_preloader_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_konqy_preloader_la_LIBADD = $(LIB_KSYCOCA)

METASOURCES = AUTO

KonquerorIface_DIR =  $(srcdir)/..

servicesdir = $(kde_servicesdir)/kded
services_DATA = konqy_preloader.desktop

autostartdir = $(datadir)/autostart
autostart_DATA = konqy_preload.desktop

####### Build rules
preloader.lo: ../konq_settingsxt.h
