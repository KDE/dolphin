# SPDX-FileCopyrightText: 2019-2023 KDE Contributors
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

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
