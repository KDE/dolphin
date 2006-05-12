
add_subdirectory( tiles ) 
add_subdirectory( indicators ) 
add_subdirectory( actions ) 



########### install files ###############


kde4_install_icons( ${ICON_INSTALL_DIR} crystalsvg )




#original Makefile.am contents follow:

#KDE_ICON = AUTO
#
#SUBDIRS = tiles indicators actions
