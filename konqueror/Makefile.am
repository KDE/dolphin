
INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kcontrol/input $(all_includes)

SUBDIRS = . plugins kfmexec client iconview dirtree listview pics

lib_LTLIBRARIES = libkonqueror.la konqueror.la

rcdir = $(kde_datadir)/konqueror
rc_DATA = konqueror.rc

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = konqueror.desktop

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = konq_plugin.desktop konq_metaview.desktop

METASOURCES = AUTO

include_HEADERS = KonquerorIface.h

libkonqueror_la_SOURCES = \
			  konq_propsview.cc konq_propsmainview.cc konq_htmlsettings.cc \
			  konq_events.cc \
			  konq_bgnddlg.cc \
		          konq_factory.cc
#			  konq_metadata.cc

libkonqueror_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -module -no-undefined

libkonqueror_la_LIBADD = ../libkonq/libkonq.la -lkparts

bin_PROGRAMS = konqueror

konqueror_la_SOURCES = konq_main.cc KonquerorIface.skel konq_mainview.skel \
		    konq_mainview.cc \
		    konq_run.cc konq_childview.cc konq_viewmgr.cc \
                    konq_misc.cc \
		    konq_frame.cc \
		    konq_actions.cc \
		    konq_profiledlg.cc

konqueror_la_LDFLAGS = $(KDE_RPATH) $(all_libraries) -module -avoid-version
konqueror_la_LIBADD = libkonqueror.la $(LIB_KIMGIO)

konqueror_SOURCES = dummy.cc
konqueror_LDADD = konqueror.la
konqueror_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy.cc:
	echo > dummy.cc

messages: rc.cpp
	$(EXTRACTRC) */*.rc >> rc.cpp
	$(XGETTEXT) -kaliasLocal -o $(podir)/konqueror.pot rc.cpp *.h *.cc *view/*h *view/*cc

check_PROGRAMS = linktest
#using $(konqueror_SOURCES) below doesn't work (am_edit)
linktest_SOURCES = konq_main.cc KonquerorIface.skel konq_mainview.skel \
                   konq_mainview.cc \
                   konq_run.cc konq_childview.cc konq_viewmgr.cc \
                   konq_misc.cc \
                   konq_frame.cc \
                   konq_actions.cc \
                   konq_profiledlg.cc
linktest_LDADD = $(lib_LTLIBRARIES) \
	$(top_builddir)/konqueror/htmlview/libkonqhtmlview.la \
	$(top_builddir)/konqueror/iconview/libkonqiconview.la \
	$(top_builddir)/konqueror/listview/libkonqlistview.la \
	$(top_builddir)/konqueror/dirtree/libkonqdirtree.la $(LIB_KDECORE)

linktest_LDFLAGS = $(konqueror_LDFLAGS)

