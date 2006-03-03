kde4_header()

add_subdirectory( tiles ) 
add_subdirectory( indicators ) 
add_subdirectory( actions ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_install_icons( ${ICON_INSTALL_DIR} crystalsvg )

kde4_footer()



#original Makefile.am contents follow:

#KDE_ICON = AUTO
#
#SUBDIRS = tiles indicators actions
