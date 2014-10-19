cmake_minimum_required(VERSION 2.8.12)

project(Dolphin)

# ECM setup
find_package(ECM 1.2.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(DOLPHIN_VERSION "4.97.0")

include(ECMSetupVersion)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

ecm_setup_version(${DOLPHIN_VERSION} VARIABLE_PREFIX DOLPHINPRIVATE
                  SOVERSION 5
)

find_package(KF5 REQUIRED COMPONENTS DocTools Init KCMUtils KDELibs4Support NewStuff)
find_package(KF5 COMPONENTS Activities)

find_package(Phonon4Qt5 CONFIG REQUIRED)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${Dolphin_SOURCE_DIR}")
    find_package(KF5Konq CONFIG REQUIRED)
else()
    include_directories(${LibKonq_SOURCE_DIR}/src)
endif()

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
    set(HAVE_BALOO TRUE)
endif()

add_subdirectory(src)
add_subdirectory(docs)

# TODO Remove the if/endif lines if Dolphin is split.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${Dolphin_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
