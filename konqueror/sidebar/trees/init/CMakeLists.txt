
add_subdirectory( remote ) 
add_subdirectory( services ) 



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqsidebartng/kicker_entries FILES home.desktop root.desktop history.desktop bookmarks.desktop                   services.desktop remote.desktop )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/dirtree FILES history_module.desktop dirtree_module.desktop bookmarks_module.desktop )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/add FILES virtualfolderadd.desktop  )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/entries FILES home.desktop root.desktop history.desktop bookmarks.desktop                   services.desktop remote.desktop system.desktop )




#original Makefile.am contents follow:

#SUBDIRS = remote services
#
#dataEntry_DATA = home.desktop root.desktop history.desktop bookmarks.desktop \
#                 services.desktop remote.desktop system.desktop
#dataEntrydir = $(kde_datadir)/konqsidebartng/entries
#
#dataKickerEntries_DATA = home.desktop root.desktop history.desktop bookmarks.desktop \
#                 services.desktop remote.desktop
#dataKickerEntriesdir = $(kde_datadir)/konqsidebartng/kicker_entries
#
#
#dataAdd_DATA = virtualfolderadd.desktop 
#dataAdddir = $(kde_datadir)/konqsidebartng/add
#
#dataPlugins_DATA = history_module.desktop dirtree_module.desktop bookmarks_module.desktop
#dataPluginsdir = $(kde_datadir)/konqsidebartng/dirtree
