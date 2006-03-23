kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/tiles FILES bluemorning.png canvas.png kde4ever.png               kenwimer.png noise.png paper_flieder.png redfiber.png )

kde4_footer()



#original Makefile.am contents follow:

#tilesdir = $(kde_datadir)/konqueror/tiles
#tiles_DATA = bluemorning.png canvas.png kde4ever.png \
#             kenwimer.png noise.png paper_flieder.png redfiber.png
