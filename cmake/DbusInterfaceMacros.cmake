macro (generate_and_install_dbus_interface main_project_target header_file output_xml_file)
    qt5_generate_dbus_interface(
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
