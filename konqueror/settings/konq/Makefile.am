INCLUDES = -I$(top_srcdir)/libkonq $(all_includes)
AM_LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_konq.la

kcm_konq_la_SOURCES = behaviour.cpp desktop.cpp rootopts.cpp \
		fontopts.cpp previews.cpp browser.cpp \
		uiserver.stub main.cpp

kcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KSYCOCA)

kcm_konq_la_DATA = overlapping.png onlyone.png
kcm_konq_ladir = $(kde_datadir)/kcontrol/pics

kcmkonq_data_DATA = filebehavior.desktop fileappearance.desktop filepreviews.desktop
kcmkonq_datadir = $(kde_appsdir)/.hidden

kcmkonq_datadesk_DATA = desktopbehavior.desktop desktop.desktop
kcmkonq_datadeskdir = $(kde_appsdir)/Settings/Desktop

kcmkonq_data2_DATA = desktoppath.desktop
kcmkonq_data2dir = $(kde_appsdir)/Settings/System

kcmkonq_data3_DATA = filebrowser.desktop
kcmkonq_data3dir = $(kde_appsdir)/Settings/Components

uiserver_DIR = $(kde_includes)/kio

EXTRA_DIST = $(kcm_konq_la_DATA) $(kcmkonq_data_DATA)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kcmkonq.pot

