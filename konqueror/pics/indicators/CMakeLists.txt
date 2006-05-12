kde4_header()



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqueror/pics FILES indicator_connect.png  	indicator_noconnect.png indicator_empty.png indicator_viewactive.png )

kde4_footer()



#original Makefile.am contents follow:

#picsdir = $(kde_datadir)/konqueror/pics
#pics_DATA = indicator_connect.png \
#	indicator_noconnect.png indicator_empty.png indicator_viewactive.png
#
