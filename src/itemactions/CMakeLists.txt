
# movetonewfolderitemaction plugin

kcoreaddons_add_plugin(movetonewfolderitemaction
    SOURCES movetonewfolderitemaction.cpp movetonewfolderitemaction.h
    INSTALL_NAMESPACE "kf6/kfileitemaction")

target_link_libraries(movetonewfolderitemaction
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets)


if(NOT WIN32)
    # setfoldericon plugin

    kcoreaddons_add_plugin(setfoldericonitemaction
        SOURCES setfoldericonitemaction.cpp setfoldericonitemaction.h ../dolphindebug.h ../dolphindebug.cpp
        INSTALL_NAMESPACE "kf6/kfileitemaction")

    target_link_libraries(setfoldericonitemaction
        KF6::I18n
        KF6::KIOCore
        KF6::KIOWidgets)
endif()
