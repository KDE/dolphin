INCLUDES  = $(MICO_INCLUDES) $(all_includes) -I$(idldir)
LDFLAGS   = $(all_libraries) $(MICO_LDFLAGS) $(KDE_RPATH)
LDADD = -lkparts -lkpartsui -lkom $(LIB_KIO) $(LIB_KHTML) $(LIB_KDEUI) \
	$(LIBMICO)

SUBDIRS = pics

####### Files

bin_PROGRAMS = konqueror

# Source files, please keep sorted.
konqueror_SOURCES =  kbookmark.cc kbrowser.cc \
	kfmguiprops.cc kfmpaths.cc kfmpopup.cc kfmrun.cc kfmviewprops.cc \
	kiconcontainer.cc \
	konq_baseview.cc konq_htmlview.cc konq_iconview.cc \
	konq_main.cc konq_mainview.cc konq_mainwindow.cc konqueror.cc \
	konq_treeview.cc konq_partview.cc \
	xview.cc

# noinst_HEADERS = ... to be completed  <= *.h? (Simon)
# No wildcards. We'll put the full list here later (useful for make dist only) (David)

METASOURCES = kbookmark.moc kbrowser.moc \
	kfmpopup.moc kfmrun.moc \
	kiconcontainer.moc \
	konq_htmlview.moc konq_iconview.moc \
	konq_main.moc konq_mainview.moc konq_mainwindow.moc \
	konq_partview.moc konq_treeview.moc

kdelnk_DATA = konqueror_part.kdelnk
kdelnkdir = $(kde_datadir)/../partlnk

DISTCLEANFILES = $(METASOURCES) konqueror.cc konqueror.h

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocale -ki18n -x $(includedir)/kde.pot $(konqueror_SOURCES) $(noinst_HEADERS) && mv messages.po ../po/konqueror.pot

####### Build rules

IDL_FLAGS = --query-server-for-narrow --relative-paths -I$(top_srcdir)/kom -I$(top_srcdir)/partsui -I$(top_srcdir)/parts -I$(idldir)

konqueror.cc: konqueror.h
konqueror.h: $(srcdir)/konqueror.idl
	  $(IDL) $(IDL_FLAGS) $(srcdir)/konqueror.idl

kfmrun.cc: konqueror.h
konq_baseview.cc: konqueror.h
konq_baseview.h: konqueror.h
konq_main.h: konqueror.h
konq_mainview.h: konqueror.h
konq_mainwindow.h: konqueror.h
