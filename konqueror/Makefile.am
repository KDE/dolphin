INCLUDES  = $(MICO_INCLUDES) -I$(idldir) -I$(top_srcdir)/libkonq $(all_includes)
LDFLAGS   = $(MICO_LDFLAGS) $(KDE_RPATH) $(all_libraries)
LDADD = -lkparts $(LIB_KFILE) $(LIB_KHTML) $(top_builddir)/libkonq/libkonq.la 

SUBDIRS = client pics Templates

data_DATA = directory.trash directory.autostart directory.templates directory.desktop
datadir = $(kde_datadir)/konqueror

service_DATA = konqueror.desktop
servicedir = $(kde_servicesdir)

EXTRA_DIST = $(data_DATA) $(service_DATA)

####### Files

bin_PROGRAMS = konqueror

# Source files, please keep sorted.
konqueror_SOURCES =  kbrowser.cc kfmrun.cc \
	konq_baseview.cc konq_childview.cc konq_frame.cc \
	konq_htmlview.cc konq_iconview.cc konq_kfmview.cc \
	konq_main.cc konq_mainview.cc konq_mainwindow.cc \
	konq_partview.cc konq_plugins.cc \
	konq_propsmainview.cc konq_propsview.cc \
	konq_topwidget.cc konq_treeview.cc konq_txtview.cc \
	konqueror.cc

noinst_HEADERS = kbrowser.h kfmrun.h \
	konq_baseview.h konq_childview.h konq_frame.h \
        konq_htmlview.h konq_iconview.h konq_kfmview.h \
        konq_main.h konq_mainview.h konq_mainwindow.h \
        konq_partview.h konq_plugins.h \
        konq_propsmainview.h konq_propsview.h \
        konq_topwidget.h konq_treeview.h konq_txtview.h \
        konqueror.h \
	mousemode.h konq_defaults.h

METASOURCES = kbrowser.moc kfmrun.moc \
	konq_childview.moc konq_frame.moc \
	konq_htmlview.moc konq_iconview.moc \
	konq_main.moc konq_mainview.moc konq_mainwindow.moc \
	konq_partview.moc konq_topwidget.moc konq_treeview.moc konq_txtview.moc

DISTCLEANFILES = $(METASOURCES) konqueror.cc konqueror.h

idl_DATA = konqueror.idl

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocale -ki18n -x $(includedir)/kde.pot *.cc *.h && mv messages.po ../po/konqueror.pot

####### Build rules

IDL_FLAGS = --query-server-for-narrow --relative-paths --any -I$(idldir)

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/konqueror.idl
	  $(IDL) $(IDL_FLAGS) $(srcdir)/konqueror.idl

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

