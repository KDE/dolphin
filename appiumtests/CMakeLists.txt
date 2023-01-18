# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

if("${QT_MAJOR_VERSION}" STREQUAL "6" AND NOT Qt${QT_MAJOR_VERSION}_VERSION VERSION_GREATER_EQUAL "6.5.0")
    # Before Qt 6.5 the AbstractButtons didn't trigger properly at all.
    return()
endif()

if(NOT BUILD_TESTING OR NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

find_package(SeleniumWebDriverATSPI)
set_package_properties(SeleniumWebDriverATSPI PROPERTIES
    DESCRIPTION "Server component for selenium tests using Linux accessibility infrastructure"
    PURPOSE "Needed for GUI tests"
    URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
    TYPE OPTIONAL
)
if(NOT SeleniumWebDriverATSPI_FOUND)
    return()
endif()

add_test(
    NAME dolphintest
    COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/dolphintest.py
)
set_tests_properties(dolphintest PROPERTIES TIMEOUT 300 ENVIRONMENT "TEST_WITH_KWIN_WAYLAND=1")
