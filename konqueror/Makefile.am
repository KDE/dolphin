INCLUDES  = $(MICO_INCLUDES) -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS   = $(all_libraries) $(KDE_RPATH) $(MICO_LDFLAGS)
LDADD = -lkparts -lkded $(LIB_KFILE) $(LIB_KHTML) $(top_builddir)/libkonq/libkonq.la 
IDL_FLAGS = --query-server-for-narrow --relative-paths --any -I$(idldir)

SUBDIRS = client pics Templates

data_DATA = directory.trash directory.autostart directory.templates directory.desktop
datadir = $(kde_datadir)/konqueror

service_DATA = konqueror.desktop
servicedir = $(kde_servicesdir)

servicetype_DATA = konqkomplugin.desktop
servicetypedir = $(kde_servicetypesdir)

EXTRA_DIST = $(data_DATA) $(service_DATA) $(servicetype_DATA)

####### Files

bin_PROGRAMS = konqueror

# Source files, please keep sorted.
konqueror_SOURCES =  browser.idl konq_mainwindow.cc kfmrun.cc \
	konq_baseview.cc konq_childview.cc konq_frame.cc \
	konq_htmlview.cc konq_iconview.cc \
	konq_main.cc konq_mainview.cc \
	konq_plugins.cc konq_propsmainview.cc konq_propsview.cc \
	konq_searchdia.cc konq_topwidget.cc konq_treeview.cc konq_txtview.cc \
	ksplitter.cc konqueror.idl

noinst_HEADERS = browser.h kfmrun.h \
	konq_baseview.h konq_childview.h konq_frame.h \
        konq_htmlview.h konq_iconview.h \
        konq_main.h konq_mainview.h konq_mainwindow.h \
        konq_plugins.h konq_propsmainview.h konq_propsview.h \
        konq_searchdia.h konq_topwidget.h konq_treeview.h konq_txtview.h \
        konqueror.h \
	ksplitter.h mousemode.h konq_defaults.h

METASOURCES = kfmrun.moc \
	konq_childview.moc konq_frame.moc \
	konq_htmlview.moc konq_iconview.moc \
	konq_main.moc konq_mainview.moc konq_mainwindow.moc \
	konq_searchdia.moc konq_topwidget.moc konq_treeview.moc \
	ksplitter.moc konq_txtview.moc

DISTCLEANFILES = $(METASOURCES) browser.cc browser.h konqueror.cc konqueror.h

idl_DATA = konqueror.idl

browser_IDLDIR = $(idldir)

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocale -ki18n -x $(includedir)/kde.pot *.cc *.h && mv messages.po ../po/konqueror.pot

####### Build rules

konqueror.h: browser.h

kfmrun.cc: konqueror.h
konq_baseview.cc: konqueror.h
konq_baseview.h: konqueror.h
konq_main.h: konqueror.h
konq_mainview.h: konqueror.h
konq_mainwindow.h: konqueror.h

# Because konqueror.cc is duplicated in client/, the TAGS file is invalid
# Solution : remove inclusion of client/TAGS
etags: tags
	mv TAGS TAGS_tmp && \
	  head -$$[`wc -l <TAGS_tmp`-2] TAGS_tmp > TAGS && \
	    rm -f TAGS_tmp

