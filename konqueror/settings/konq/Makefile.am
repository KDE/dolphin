
INCLUDES = $(all_includes) 
LDFLAGS =  $(all_libraries) $(KDE_RPATH)
LDADD =    $(LIB_KDEUI) 

bin_PROGRAMS = kcmkonq
METASOURCES = htmlopts.moc khttpoptdlg.moc miscopts.moc
noinst_HEADERS = htmlopts.h khttpoptdlg.h miscopts.h

kcmkonq_SOURCES = main.cpp htmlopts.cpp khttpoptdlg.cpp miscopts.cpp

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kcmkonq_SOURCES) && mv messages.po ../../po/kcmkonq.pot

