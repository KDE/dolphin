remove_definitions(-DTRANSLATION_DOMAIN=\"dolphin\")
add_definitions(-DTRANSLATION_DOMAIN=\"dolphin_servicemenuinstaller\")

add_executable(servicemenuinstaller servicemenuinstaller.cpp)
target_link_libraries(servicemenuinstaller PRIVATE
    Qt6::Core
    Qt6::Gui
    KF6::I18n
    KF6::CoreAddons
)

if(HAVE_PACKAGEKIT)
    target_link_libraries(servicemenuinstaller PRIVATE PK::packagekitqt6)
endif()
install(TARGETS servicemenuinstaller ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
