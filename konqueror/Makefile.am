
INCLUDES  = $(MICO_INCLUDES) $(all_includes) -I$(idldir)
LDFLAGS   = $(all_libraries) $(MICO_LDFLAGS) $(KDE_RPATH)
LDADD = -lkparts -lkpartsui -lkom $(LIB_KIO) $(LIB_KHTML) $(LIB_KDEUI) \
	$(LIBMICO) 

####### Files

bin_PROGRAMS = konqueror

konqueror_SOURCES =  konq_main.cc konq_mainwindow.cc konqueror.cc \
	kbookmark.cc konq_iconview.cc kiconcontainer.cc \
	kfmpaths.cc xview.cc kfmguiprops.cc konq_baseview.cc \
	konq_mainview.cc kfmpopup.cc kfmviewprops.cc

# noinst_HEADERS = ... to be completed

METASOURCES = konq_main.moc konq_mainwindow.moc \
	kbookmark.moc konq_iconview.moc kiconcontainer.moc \
	konq_mainview.moc kfmpopup.moc

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

konq_baseview.h: konqueror.h
konq_main.h: konqueror.h
konq_mainview.h: konqueror.h
konq_mainwindow.h: konqueror.h