
INCLUDES= -I.. -I$(top_srcdir)/libkonq $(all_includes)
kfmclient_la_LDFLAGS = $(all_libraries) -module -avoid-version
kfmclient_la_LIBADD =  $(top_builddir)/libkonq/libkonq.la

kfmclient_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kfmclient_LDADD = kfmclient.la

bin_PROGRAMS = kfmclient
lib_LTLIBRARIES = kfmclient.la

kfmclient_la_SOURCES = kfmclient.cc KonquerorIface.stub KDesktopIface.stub
noinst_HEADERS = kfmclient.h
METASOURCES = AUTO

KonquerorIface_DIR =  $(srcdir)/..
KDesktopIface_DIR = $(top_srcdir)/kdesktop

kfmclient_SOURCES = dummy.cc

dummy.cc:
	echo > dummy.cc

messages:
	$(XGETTEXT) -kaliasLocal *.h *.cc -o $(podir)/kfmclient.pot

