
INCLUDES  = -I$(top_srcdir)/kom -I$(top_builddir)/kom \
	-I$(top_srcdir)/parts -I$(top_builddir)/parts \
	-I$(top_srcdir)/partsui -I$(top_builddir)/partsui \
	$(MICO_INCLUDES) $(all_includes) -I$(idldir)
LDFLAGS   = $(all_libraries) $(MICO_LDFLAGS) $(KDE_RPATH)
LDADD = $(top_builddir)/parts/libkparts.la $(top_builddir)/partsui/libkpartsui.la \
	$(top_builddir)/kom/libkom.la -lkdeutil $(LIB_KHTML) -lkio $(LIB_KDEUI) \
	$(LIBMICO) 

####### Files

bin_PROGRAMS = konqueror

konqueror_SOURCES = kfm_main.cc kfm_mainwindow.cc kfm.cc \
		    kbookmark.cc kfmicons.cc kiconcontainer.cc \
		    kbrowser.cc kfmbrowser.cc kfmpaths.cc \
		    kfmfinder.cc kfmrun.cc xview.cc \
		    kfmgui.cc kfmview.cc kfm_part.cc \
		    kfmpopup.cc
noinst_HEADERS = kfm_main.h kfm_mainwindow.h kfm.h \
		 kbookmark.h kfmicons.h kiconcontainer.h \
		 kbrowser.h kfmbrowser.h kfmpaths.h \
		 kfmfinder.h kfmrun.h xview.h \
		 kfmgui.h kfmview.h kfm_part.h kfm_abstract_gui.h \
		 kfmpopup.h
SRCMETA = kfm_main.moc kfm_mainwindow.moc \
	  kbookmark.moc kbrowser.moc kfmbrowser.moc \
	  kfmfinder.moc kfmgui.moc kfmicons.moc \
	  kfmrun.moc kfmview.moc kiconcontainer.moc \
	  kfm_part.moc kfmpopup.moc

kdelnk_DATA = konqueror_part.kdelnk
kdelnkdir = $(kde_datadir)/../partlnk

DISTCLEANFILES = $(SRCMETA) kfm.cc kfm.h

messages:
	$(XGETTEXT) -C -ktranslate -kaliasLocale -ki18n -x $(includedir)/kde.pot $(konqueror_SOURCES) $(noinst_HEADERS) && mv messages.po ../po/konqueror.pot

####### Implicit rules

SUFFIXES = .h .moc

.h.moc:
	$(MOC) $< -o $@ 

####### Build rules

IDL_FLAGS = --query-server-for-narrow --relative-paths -I$(top_srcdir)/kom -I$(top_srcdir)/partsui -I$(top_srcdir)/parts -I$(idldir)

kfm.cc: kfm.h
kfm.h: $(srcdir)/kfm.idl
	  $(IDL) $(IDL_FLAGS) $(srcdir)/kfm.idl

kfm_main.cc: kfm_main.moc kfm.h
kfm_mainwindow.cc: kfm_mainwindow.moc kfm.h
kfm_part.cc: kfm_part.moc kfm.h
kbookmark.cc: kbookmark.moc
kbrowser.cc: kbrowser.moc
kfmbrowser.cc: kfmbrowser.moc
kfmfinder.cc: kfmfinder.moc
kfmgui.cc: kfmgui.moc
kfmicons.cc: kfmicons.moc
kfmrun.cc: kfmrun.moc
kfmview.cc: kfmview.moc
kiconcontainer.cc: kiconcontainer.moc
kfmpopup.cc: kfmpopup.moc
