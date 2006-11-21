
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  preview.png DESTINATION ${DATA_INSTALL_DIR}/dolphin/icons/hicolor/128x128/actions )



#original Makefile.am contents follow:

#iconsdir = $(kde_datadir)/dolphin/icons/hicolor/128x128/actions
#icons_DATA = preview.png
#
