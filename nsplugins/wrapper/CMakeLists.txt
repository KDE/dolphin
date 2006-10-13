include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nswrapper_LIB_SRCS wrapper.c )

kde4_automoc(nswrapper ${nswrapper_LIB_SRCS})

kde4_add_library(nswrapper SHARED ${nswrapper_LIB_SRCS})

target_link_libraries(nswrapper  ${KDE4_KDECORE_LIBS} )

set_target_properties(nswrapper PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS nswrapper  DESTINATION ${LIB_INSTALL_DIR} )

