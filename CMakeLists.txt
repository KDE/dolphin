cmake_minimum_required(VERSION 2.8.12)

project(Dolphin)

set(DOLPHIN_VERSION "14.12.95")

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.7.0")
set(ECM_MIN_VERSION "1.6.0")

# ECM setup
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMPackageConfigHelpers)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

ecm_setup_version(${DOLPHIN_VERSION} VARIABLE_PREFIX DOLPHINVCS
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfigVersion.cmake"
                  SOVERSION 5
)

ecm_setup_version(${DOLPHIN_VERSION} VARIABLE_PREFIX DOLPHINPRIVATE
                  SOVERSION 5
)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    Widgets
    Gui
    DBus
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    DocTools
    Init
    KCMUtils
    NewStuff
    CoreAddons
    I18n
    DBusAddons
    Bookmarks
    Config
    KIO
    Parts
    Solid
    IconThemes
    Completion
    TextEditor
    WindowSystem
    Notifications
)
find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS
    Activities
)

find_package(Phonon4Qt5 CONFIG REQUIRED)

find_package(KF5Baloo 4.97)
set_package_properties(KF5Baloo PROPERTIES DESCRIPTION "Baloo Core libraries"
                       URL "http://www.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "For adding desktop-wide search and tagging support to dolphin"
                      )

find_package(KF5BalooWidgets 4.97)
set_package_properties(KF5BalooWidgets PROPERTIES DESCRIPTION "Baloos Widgets"
                       URL "http://www.kde.org"
                       TYPE OPTIONAL
                      )

find_package(KF5FileMetaData 4.97)
set_package_properties(KF5FileMetaData PROPERTIES
                       URL "https://projects.kde.org/kfilemetadata"
                       TYPE OPTIONAL
                       PURPOSE "For accessing file metadata labels"
                      )

if (KF5Baloo_FOUND AND KF5BalooWidgets_FOUND AND KF5FileMetaData_FOUND)
    message(STATUS "Baloo packages are found")
    set(HAVE_BALOO TRUE)
else()
    find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
        KDELibs4Support # for KFileMetaDataWidget
    )
endif()

add_subdirectory(src)
add_subdirectory(doc)

# CMake files
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/DolphinVcs")

ecm_configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/DolphinVcsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT DolphinVcsTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE DolphinVcsTargets.cmake
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_version.h"
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/Dolphin"
    COMPONENT Devel
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

