
project(libkonq)

add_subdirectory( pics )
add_subdirectory( favicons )
add_subdirectory( tests )

include_directories( ${ZLIB_INCLUDE_DIR}  )


########### next target ###############

# Please use .cpp for new files
set(konq_LIB_SRCS
   konq_popupmenu.cc
   knewmenu.cc
   konq_xmlguiclient.cc
   kfileivi.cc
   konq_iconviewwidget.cc
   konq_settings.cc
   konq_operations.cc
   konq_dirpart.cc
   konq_propsview.cc
   konq_events.cc
   konq_bgnddlg.cc
   konq_undo.cc
   konq_undoadaptor.cpp
   konq_historymgr.cc
   konq_historymgr_adaptor.cpp
   konq_historyentry.cpp
   konq_pixmapprovider.cc
   kivdirectoryoverlay.cc
   konq_faviconmgr.cc
   konq_filetip.cc
   konqmimedata.cpp )

kde4_automoc(${konq_LIB_SRCS})

kde4_add_library(konq SHARED ${konq_LIB_SRCS})

target_link_libraries(konq ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} ${KDE4_KDEFX_LIBS} ${KDE4_KDESU_LIBS})

set_target_properties(konq PROPERTIES VERSION 6.0.0 SOVERSION 6 )
install(TARGETS konq  DESTINATION ${LIB_INSTALL_DIR} )

########### next target ###############

set(konq_sound_LIB_SRCS konq_sound.cc)
kde4_automoc(${konq_sound_LIB_SRCS})
kde4_add_plugin(konq_sound ${konq_sound_LIB_SRCS})
kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} konq_sound)
target_link_libraries(konq_sound ${KDE4_KDECORE_LIBS} ${KDE4_PHONONCORE_LIBRARY})
install(TARGETS konq_sound  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES directory_bookmarkbar.desktop   DESTINATION  ${DATA_INSTALL_DIR}/kbookmark )
install( FILES
 konq_popupmenu.h
 knewmenu.h
 kfileivi.h
 konq_iconviewwidget.h
 konq_defaults.h
 konq_settings.h
 konq_operations.h
 libkonq_export.h
 konq_dirpart.h
 konq_propsview.h
 konq_events.h
 konq_undo.h
 konq_historymgr.h
 konq_historymgr_adaptor.h
 konq_historyentry.h
 konq_pixmapprovider.h
 kivdirectoryoverlay.h
 konq_faviconmgr.h
 konq_xmlguiclient.h
 konqbookmarkmanager.h
 konq_filetip.h
 konqmimedata.h
DESTINATION ${INCLUDE_INSTALL_DIR}
)
install( FILES konqpopupmenuplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )


