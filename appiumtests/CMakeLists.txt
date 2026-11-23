# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

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
if(NOT SeleniumWebDriverATSPI_FOUND AND NOT DEFINED ENV{KDECI_BUILD})
    return()
endif()

function(add_dolphin_appium_test name)
    add_test(
        NAME ${name}
        COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py
    )
    set_tests_properties(${name} PROPERTIES
        TIMEOUT 180
        RESOURCE_LOCK appium
        ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin;TEST_WITH_VIDEO_RECORDER=0;PYTHONDONTWRITEBYTECODE=1"
    )
endfunction()

add_dolphin_appium_test(locationbartest)
add_dolphin_appium_test(filterbartest)
add_dolphin_appium_test(searchbartest)
add_dolphin_appium_test(columnsviewtest)
