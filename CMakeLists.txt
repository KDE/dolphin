cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(Dolphin VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.92.0")

# ECM setup
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGenerateDBusServiceFile)
include(ECMConfiguredInstall)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX DOLPHIN
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/dolphin_version.h"
)

ecm_setup_version("5.0.0" VARIABLE_PREFIX DOLPHINVCS
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfigVersion.cmake"
                  SOVERSION 5
)

ecm_setup_version("5.0.0" VARIABLE_PREFIX DOLPHINPRIVATE
                  SOVERSION 5
)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    Widgets
    Gui
    DBus
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
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
    TextWidgets
    Notifications
    Crash
    WindowSystem
    WidgetsAddons
)

find_package(KUserFeedback 1.2.0)
set_package_properties(KUserFeedback
        PROPERTIES TYPE OPTIONAL
        PURPOSE "Used for submission of telemetry data"
        )

if(KUserFeedback_FOUND)
    set(HAVE_KUSERFEEDBACK TRUE)
endif()

find_package(KF5 ${KF5_MIN_VERSION} OPTIONAL_COMPONENTS
    Activities
    DocTools
)
set_package_properties(KF5Activities PROPERTIES DESCRIPTION "KActivities libraries"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "For tracking which folders are frequently accessed on a Plasma desktop"
                      )

find_package(Phonon4Qt${QT_MAJOR_VERSION} CONFIG REQUIRED)

find_package(PackageKitQt${QT_MAJOR_VERSION})
set_package_properties(PackageKitQt${QT_MAJOR_VERSION}
        PROPERTIES DESCRIPTION "Software Manager integration"
        TYPE OPTIONAL
        PURPOSE "Used in the service menu installer"
        )
if(PackageKitQt${QT_MAJOR_VERSION}_FOUND)
    set(HAVE_PACKAGEKIT TRUE)
endif()

find_package(KF5Baloo ${KF5_MIN_VERSION})
set_package_properties(KF5Baloo PROPERTIES DESCRIPTION "Baloo Core libraries"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "For adding desktop-wide search and tagging support to dolphin"
                      )

find_package(KF5BalooWidgets 19.07.70)
set_package_properties(KF5BalooWidgets PROPERTIES DESCRIPTION "Baloos Widgets"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                      )

find_package(KF5FileMetaData ${KF5_MIN_VERSION})
set_package_properties(KF5FileMetaData PROPERTIES
                       URL "https://projects.kde.org/kfilemetadata"
                       TYPE OPTIONAL
                       PURPOSE "For accessing file metadata labels"
                      )

if (KF5Activities_FOUND)
    set(HAVE_KACTIVITIES TRUE)
endif()

if (KF5Baloo_FOUND AND KF5BalooWidgets_FOUND AND KF5FileMetaData_FOUND)
    message(STATUS "Baloo packages are found")
    set(HAVE_BALOO TRUE)
else()
    message(WARNING "Baloo packages not found. They are needed for the metadata features of Dolphin (including the information panel).")
endif()

# TODO: drop HAVE_TERMINAL once we are sure the terminal panel works on Windows too.
if(WIN32)
    set(HAVE_TERMINAL FALSE)
else()
    set(HAVE_TERMINAL TRUE)
endif()

ecm_set_disabled_deprecation_versions(SHOW_DEPRECATIONS
    QT 5.15
    KF 5.90
    KSERVICE 5.89 # We use KServiceTypeTrader in a compat code path
)

add_subdirectory(src)
add_subdirectory(doc)

# CMake files
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/DolphinVcs")

configure_package_config_file(
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

if(FLATPAK)
    ecm_generate_dbus_service_file(
        NAME org.freedesktop.FileManager1
        EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/dolphin --daemon"
        SYSTEMD_SERVICE plasma-dolphin.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
    )
else()
    ecm_generate_dbus_service_file(
        NAME org.freedesktop.FileManager1
        EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/dolphin --daemon"
        SYSTEMD_SERVICE plasma-dolphin.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
        RENAME org.kde.dolphin.FileManager1.service
    )
endif()

ecm_install_configured_files(INPUT plasma-dolphin.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

ecm_qt_install_logging_categories(
        EXPORT DOLPHIN
        FILE dolphin.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

