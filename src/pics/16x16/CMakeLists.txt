
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  preview.png editurl.png DESTINATION ${DATA_INSTALL_DIR}/dolphin/icons/hicolor/16x16/actions )



#original Makefile.am contents follow:

#iconsdir = $(kde_datadir)/dolphin/icons/hicolor/16x16/actions
#icons_DATA = preview.png editurl.png
#
