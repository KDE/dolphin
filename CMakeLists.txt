cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(Dolphin VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.4.0")
set(KF6_MIN_VERSION "6.2.0")

# ECM setup
find_package(ECM ${KF6_MIN_VERSION} CONFIG REQUIRED)
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
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckFunctionExists)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX DOLPHIN
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/dolphin_version.h"
)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX DOLPHINVCS
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/dolphinvcs_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DolphinVcsConfigVersion.cmake"
                  SOVERSION 6
)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX DOLPHINPRIVATE
                  SOVERSION 6
)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    Widgets
    Gui
    DBus
)

if (UNIX AND NOT APPLE AND NOT HAIKU)
    set(HAVE_X11 TRUE)
endif()

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
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
    Codecs
    GuiAddons
)

find_package(KF6UserFeedback ${KF6_MIN_VERSION})
set_package_properties(KF6UserFeedback
        PROPERTIES TYPE OPTIONAL
        PURPOSE "Used for submission of telemetry data"
        )

if(KF6UserFeedback_FOUND)
    set(HAVE_KUSERFEEDBACK TRUE)
endif()

find_package(KF6 ${KF6_MIN_VERSION} OPTIONAL_COMPONENTS
    DocTools
)

find_package(Phonon4Qt6 CONFIG REQUIRED)

find_package(PackageKitQt6)
set_package_properties(PackageKitQt6
        PROPERTIES DESCRIPTION "Software Manager integration"
        TYPE RECOMMENDED
        PURPOSE "Used in the service menu installer"
        )
if(PackageKitQt6_FOUND)
    set(HAVE_PACKAGEKIT TRUE)
endif()

find_package(KF6Baloo ${KF6_MIN_VERSION})
set_package_properties(KF6Baloo PROPERTIES DESCRIPTION "Baloo Core libraries"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "For adding desktop-wide search and tagging support to dolphin"
                      )

find_package(KF6BalooWidgets ${RELEASE_SERVICE_VERSION})
set_package_properties(KF6BalooWidgets PROPERTIES DESCRIPTION "Baloos Widgets"
                       URL "https://www.kde.org"
                       TYPE OPTIONAL
                      )

find_package(KF6FileMetaData ${KF6_MIN_VERSION})
set_package_properties(KF6FileMetaData PROPERTIES
                       URL "https://projects.kde.org/kfilemetadata"
                       TYPE OPTIONAL
                       PURPOSE "For accessing file metadata labels"
                      )

if (KF6Baloo_FOUND AND KF6BalooWidgets_FOUND AND KF6FileMetaData_FOUND)
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

# Compatibility with platforms without native fts (e.g. musl)
check_include_files("sys/types.h;sys/stat.h;fts.h" HAVE_FTS_H)
if(HAVE_FTS_H AND NOT HAIKU)
    check_function_exists(fts_open HAVE_FTS_OPEN)
    if(NOT HAVE_FTS_OPEN)
        check_library_exists(fts fts_open "" HAVE_LIB_FTS)
    endif()
endif()

if(HAVE_LIB_FTS)
    set(FTS_LIB fts)
else()
    set(FTS_LIB "")
endif()

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(appiumtests)

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

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

install(FILES completions/dolphin.zsh RENAME _dolphin DESTINATION ${KDE_INSTALL_ZSHAUTOCOMPLETEDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

include(KDEClangFormat)
# add clang-format target
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(KDEGitCommitHooks)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
