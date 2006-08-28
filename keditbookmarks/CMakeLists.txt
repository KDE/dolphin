include_directories( ${CMAKE_SOURCE_DIR}/libkonq  )


########### next target ###############

set(kbookmarkmerger_SRCS kbookmarkmerger.cpp )

kde4_automoc(${kbookmarkmerger_SRCS})

kde4_add_executable(kbookmarkmerger ${kbookmarkmerger_SRCS})

target_link_libraries(kbookmarkmerger  ${KDE4_KIO_LIBS} )

install(TARGETS kbookmarkmerger DESTINATION ${BIN_INSTALL_DIR})


########### next target ###############

set(keditbookmarks_KDEINIT_SRCS 
   main.cpp 
   toplevel.cpp 
   actionsimpl.cpp 
   commands.cpp 
   importers.cpp 
   dcop.cpp 
   bookmarkiterator.cpp 
   testlink.cpp 
   favicons.cpp 
   updater.cpp 
   exporters.cpp 
   bookmarkinfo.cpp 
   kebsearchline.cpp 
   bookmarkmodel.cpp 
   bookmarklistview.cpp 
   treeitem.cpp )

kde4_automoc(${keditbookmarks_KDEINIT_SRCS})



kde4_add_kcfg_files(keditbookmarks_KDEINIT_SRCS settings.kcfgc )

kde4_add_kdeinit_executable( keditbookmarks ${keditbookmarks_KDEINIT_SRCS})

target_link_libraries(kdeinit_keditbookmarks  ${KDE4_KDECORE_LIBS} konq )

install(TARGETS kdeinit_keditbookmarks  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( keditbookmarks kdeinit_keditbookmarks )
install(TARGETS keditbookmarks DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES keditbookmarksui.rc  keditbookmarks-genui.rc  DESTINATION  ${DATA_INSTALL_DIR}/keditbookmarks )

kde4_install_icons( ${ICON_INSTALL_DIR}  )
