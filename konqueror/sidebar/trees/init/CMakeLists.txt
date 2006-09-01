
add_subdirectory( remote ) 
add_subdirectory( services ) 



########### install files ###############

install( FILES home.desktop root.desktop history.desktop bookmarks.desktop                   services.desktop remote.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/kicker_entries )
install( FILES history_module.desktop dirtree_module.desktop bookmarks_module.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/dirtree )
install( FILES virtualfolderadd.desktop   DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/add )
install( FILES home.desktop root.desktop history.desktop bookmarks.desktop                   services.desktop remote.desktop system.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries )

