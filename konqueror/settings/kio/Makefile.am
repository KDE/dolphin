
INCLUDES = $(all_includes) 
LDFLAGS =  $(all_libraries) $(KDE_RPATH)
LDADD =    $(LIB_KDEUI) 

bin_PROGRAMS = kcmkio
METASOURCES = kproxydlg.moc ksmboptdlg.moc useragentdlg.moc kcookiesdlg.moc
noinst_HEADERS = kproxydlg.h ksmboptdlg.h useragentdlg.h kcookiesdlg.h

kcmkio_SOURCES = main.cpp kproxydlg.cpp ksmboptdlg.cpp useragentdlg.cpp kcookiesdlg.cpp

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kcmkio_SOURCES) && mv messages.po ../../po/kcmkio.pot

