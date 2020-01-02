cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "20")
set (RELEASE_SERVICE_VERSION_MINOR "03")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(Dolphin VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.67.0")

# ECM setup
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

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
    TextWidgets
    Notifications
    Crash
    WindowSystem
)
find_package(KF5 ${KF5_MIN_VERSION} OPTIONAL_COMPONENTS
    Activities
)
set_package_properties(KF5Activities PROPERTIES DESCRIPTION "KActivities libraries"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "For tracking which folders are frequently accessed on a Plasma desktop"
                      )

find_package(Phonon4Qt5 CONFIG REQUIRED)

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

configure_file(org.kde.dolphin.FileManager1.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.dolphin.FileManager1.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.dolphin.FileManager1.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
install(FILES dolphin.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

