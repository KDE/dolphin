AM_CPPFLAGS = -I$(top_srcdir)/libkonq $(all_includes)
AM_LDFLAGS =  $(all_libraries)

METASOURCES = AUTO

kde_module_LTLIBRARIES = kcm_konq.la

kcm_konq_la_SOURCES = behaviour.cpp desktop.cpp rootopts.cpp \
		fontopts.cpp previews.cpp browser.cpp \
		uiserver.stub main.cpp desktopbehavior_impl.cpp \
		desktopbehavior.ui

kcm_konq_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
kcm_konq_la_LIBADD = $(LIB_KIO) $(LIB_KDEUI) $(LIB_KSYCOCA) $(LIB_KUTILS)

kcm_konq_la_DATA = overlapping.png onlyone.png
kcm_konq_ladir = $(kde_datadir)/kcontrol/pics

kcmkonq_data_DATA = filebehavior.desktop fileappearance.desktop filepreviews.desktop
kcmkonq_datadir = $(kde_appsdir)/.hidden

xdg_apps_DATA = desktopbehavior.desktop desktop.desktop desktoppath.desktop filebrowser.desktop

uiserver_DIR = $(kde_includes)/kio

EXTRA_DIST = $(kcm_konq_la_DATA) $(kcmkonq_data_DATA)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmkonq.pot

