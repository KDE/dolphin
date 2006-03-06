kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kfindpart/icons FILES  )

-./share/apps/kfindpart/icons/locolor/22x22/actions/archive.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/delete.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/idea.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/info.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/openfile.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/save.png
-./share/apps/kfindpart/icons/locolor/22x22/actions/search.png


kde4_footer()



#original Makefile.am contents follow:

#kfind_pics_datadir = $(kde_datadir)/kfindpart/icons
#kfind_pics_data_ICON = AUTO
