AM_CPPFLAGS = -DQT_NO_CAST_ASCII

bin_PROGRAMS = kfmclient
lib_LTLIBRARIES = kfmclient.la

INCLUDES= -I.. $(all_includes)
kfmclient_la_LDFLAGS = $(all_libraries) -module -avoid-version
kfmclient_la_LIBADD =  $(LIB_KFILE)
kfmclient_la_SOURCES = kfmclient.cc KonquerorIface.stub KDesktopIface.stub

noinst_HEADERS = kfmclient.h
METASOURCES = AUTO

KonquerorIface_DIR =  $(srcdir)/..
KDesktopIface_DIR = $(top_srcdir)/kdesktop

kfmclient_SOURCES = dummy.cc
kfmclient_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kfmclient_LDADD = kfmclient.la

dummy.cc:
	echo > dummy.cc

messages:
	$(XGETTEXT) -kaliasLocal *.h *.cc -o $(podir)/kfmclient.pot

