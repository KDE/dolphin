INCLUDES = -I$(top_srcdir)/libkonq -I$(srcdir)/.. -I$(srcdir)/../..  $(all_includes)

kde_module_LTLIBRARIES = konq_sidebartree_history.la kcm_history.la

METASOURCES = AUTO

konq_sidebartree_history_la_SOURCES = history_module.cpp history_item.cpp \
	history_settings.cpp history_settings.skel
konq_sidebartree_history_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
konq_sidebartree_history_la_LIBADD =  ../../libkonqsidebarplugin.la ../libkonq_sidebar_tree.la

kcm_history_la_SOURCES = kcmhistory.cpp history_dlg.ui history_settings.cpp history_settings.skel
kcm_history_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_history_la_LIBADD = -lkdeui $(top_builddir)/libkonq/libkonq.la

xdg_apps_DATA = kcmhistory.desktop
