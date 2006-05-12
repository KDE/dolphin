kde4_header()



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/remote/ftp FILES .directory kde_ftp.desktop )

kde4_footer()



#original Makefile.am contents follow:

#konq_sidebartree_virtuals_remote_ftp_data_DATA = .directory kde_ftp.desktop
#konq_sidebartree_virtuals_remote_ftp_datadir = $(kde_datadir)/konqsidebartng/virtual_folders/remote/ftp
#
