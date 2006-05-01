AM_CPPFLAGS = -DQT_NO_CAST_ASCII

bin_PROGRAMS= 
lib_LTLIBRARIES = 
kdeinit_LTLIBRARIES = kfmclient.la

INCLUDES= -I.. $(all_includes)
kfmclient_la_LDFLAGS = $(all_libraries) -module -avoid-version
kfmclient_la_LIBADD =  $(LIB_KIO)
kfmclient_la_SOURCES = kfmclient.cc KonquerorIface.stub KDesktopIface.stub

noinst_HEADERS = kfmclient.h
METASOURCES = AUTO

KonquerorIface_DIR =  $(srcdir)/..
KDesktopIface_DIR = $(top_srcdir)/workspace/kdesktop


updatedir = $(kde_datadir)/kconf_update
update_DATA = kfmclient_3_2.upd
update_SCRIPTS = kfmclient_3_2_update.sh
