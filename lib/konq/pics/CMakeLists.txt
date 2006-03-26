kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/pics FILES thumbnailfont_7x4.png arrow_topleft.png arrow_topright.png arrow_bottomleft.png arrow_bottomright.png )

kde4_footer()



#original Makefile.am contents follow:

#libkonq_pics_datadir = $(kde_datadir)/konqueror/pics
#libkonq_pics_data_DATA = thumbnailfont_7x4.png arrow_topleft.png arrow_topright.png arrow_bottomleft.png arrow_bottomright.png
