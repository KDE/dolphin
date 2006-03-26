kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services FILES .directory audiocd.desktop printsystem.desktop media.desktop settings.desktop applications.desktop )

kde4_footer()



#original Makefile.am contents follow:

#konq_sidebartree_init_services_data_DATA = .directory audiocd.desktop printsystem.desktop media.desktop settings.desktop applications.desktop
#konq_sidebartree_init_services_datadir = $(kde_datadir)/konqsidebartng/virtual_folders/services
