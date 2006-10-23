


########### next target ###############

set(kdialog_SRCS 
	kdialog.cpp 
   widgets.cpp 
   klistboxdialog.cpp 
   progressdialog.cpp )

qt4_add_dbus_adaptor( kdialog_SRCS org.kde.kdialog.ProgressDialog.xml progressdialog.h KProgressDialog )

kde4_automoc(${kdialog_SRCS})


kde4_add_executable(kdialog ${kdialog_SRCS})

target_link_libraries(kdialog  ${KDE4_KIO_LIBS} )

install(TARGETS kdialog DESTINATION ${BIN_INSTALL_DIR})

