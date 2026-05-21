# SPDX-FileCopyrightText: 2015 Ashish Bansal <bansal.ashish096@gmail.com>
# SPDX-FileCopyrightText: 2015 David Faure <faure@kde.org>
# SPDX-FileCopyrightText: 2019 Elvis Angelaccio <elvis.angelaccio@kde.org>
# SPDX-FileCopyrightText: 2022 Laurent Montel <montel@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

macro (generate_and_install_dbus_interface main_project_target header_file output_xml_file)
    qt_generate_dbus_interface(
        ${header_file}
        ${output_xml_file}
    )
    add_custom_target(
        ${output_xml_file}_target
        SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
    )
    add_dependencies(
        ${main_project_target}
        ${output_xml_file}_target
    )
endmacro ()
