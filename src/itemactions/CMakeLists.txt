
# movetonewfolderitemaction plugin

kcoreaddons_add_plugin(movetonewfolderitemaction
    SOURCES movetonewfolderitemaction.cpp movetonewfolderitemaction.h
    INSTALL_NAMESPACE "kf6/kfileitemaction")

target_link_libraries(movetonewfolderitemaction
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets)
