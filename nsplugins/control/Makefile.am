INCLUDES= $(all_includes)

KDE_ICON=AUTO

libkcm_nsplugin_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI) $(LIB_KFILE)
libkcm_nsplugin_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libkcm_nsplugin_la_METASOURCES = AUTO
libkcm_nsplugin_la_SOURCES = nsconfig.cpp

lib_LTLIBRARIES = libkcm_nsplugin.la

noinst_HEADERS = nsconfig.h

DISTCLEANFILES = $(libkcm_nsplugin_la_METASOURCES)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/nsplugin.pot

data_DATA = nsplugin.desktop
datadir = $(kde_appsdir)/Settings/Browsing/Files
