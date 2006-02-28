kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/pics FILES indicator_connect.png  	indicator_noconnect.png indicator_empty.png indicator_viewactive.png )

kde4_footer()



#original Makefile.am contents follow:

#picsdir = $(kde_datadir)/konqueror/pics
#pics_DATA = indicator_connect.png \
#	indicator_noconnect.png indicator_empty.png indicator_viewactive.png
#
