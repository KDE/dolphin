INCLUDES  = $(MICO_INCLUDES) $(all_includes) -I$(idldir)
LDFLAGS   = $(all_libraries) $(MICO_LDFLAGS) $(KDE_RPATH)
LDADD = -lkparts $(LIB_KFILE) $(LIB_KHTML) 

SUBDIRS = client pics Templates

data_DATA = directory.trash directory.autostart directory.templates directory.desktop
datadir = $(kde_datadir)/kfm

EXTRA_DIST = $(data_DATA)

# this gives better compile-time warnings.
# CXXFLAGS += -pedantic

####### Files

bin_PROGRAMS = konqueror

# Source files, please keep sorted.
konqueror_SOURCES =  kbookmarkmenu.cc kbrowser.cc \
	konq_propsmainview.cc kpopupmenu.cc kfmrun.cc kfmviewprops.cc \
	kiconcontainer.cc \
	konq_baseview.cc konq_childview.cc konq_frame.cc \
	konq_htmlview.cc konq_iconview.cc konq_kfmview.cc \
	konq_main.cc konq_mainview.cc konq_mainwindow.cc konq_plugins.cc \
	konq_partview.cc konq_treeview.cc konq_txtview.cc konqueror.cc \
	knewmenu.cc

# noinst_HEADERS = ... to be completed

METASOURCES = kbookmarkmenu.moc kbrowser.moc \
	kpopupmenu.moc kfmrun.moc \
	kiconcontainer.moc \
	konq_childview.moc konq_frame.moc \
	konq_htmlview.moc konq_iconview.moc \
	konq_main.moc konq_mainview.moc konq_mainwindow.moc \
	konq_partview.moc konq_treeview.moc konq_txtview.moc knewmenu.moc

kde_parts_DATA = konqueror_part.kdelnk

DISTCLEANFILES = $(METASOURCES) konqueror.cc konqueror.h

idl_DATA = konqueror.idl

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocale -ki18n -x $(includedir)/kde.pot $(konqueror_SOURCES) $(noinst_HEADERS) && mv messages.po ../po/konqueror.pot

####### Build rules

IDL_FLAGS = --query-server-for-narrow --relative-paths -I$(idldir)

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/konqueror.idl
	  $(IDL) $(IDL_FLAGS) $(srcdir)/konqueror.idl

kfmrun.cc: konqueror.h
kbookmarkmenu.cc: konqueror.h
konq_baseview.cc: konqueror.h
konq_baseview.h: konqueror.h
konq_main.h: konqueror.h
konq_mainview.h: konqueror.h
konq_mainwindow.h: konqueror.h
