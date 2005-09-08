AM_CPPFLAGS = $(all_includes)

kcm_filetypes_la_LIBADD	= libfiletypes.la $(LIB_KIO)
kcm_filetypes_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_filetypes.la

kcm_filetypes_la_SOURCES = control.cpp

noinst_HEADERS = filetypesview.h filetypedetails.h filegroupdetails.h \
	kservicelistwidget.h typeslistitem.h newtypedlg.h \
	kserviceselectdlg.h

bin_PROGRAMS = keditfiletype

noinst_LTLIBRARIES = libfiletypes.la
libfiletypes_la_SOURCES = filetypesview.cpp filetypedetails.cpp filegroupdetails.cpp \
		 kservicelistwidget.cpp typeslistitem.cpp newtypedlg.cpp \
		 kserviceselectdlg.cpp

keditfiletype_SOURCES = keditfiletype.cpp
keditfiletype_LDADD = libfiletypes.la $(LIB_KIO)
keditfiletype_LDFLAGS = $(KDE_RPATH) $(all_libraries)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/filetypes.pot

xdg_apps_DATA = filetypes.desktop
