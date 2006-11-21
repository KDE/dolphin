
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  preview.png DESTINATION ${DATA_INSTALL_DIR}/dolphin/icons/hicolor/32x32/actions )



#original Makefile.am contents follow:

#iconsdir = $(kde_datadir)/dolphin/icons/hicolor/32x32/actions
#icons_DATA = preview.png
#
