#=============================================================================
# SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause
#=============================================================================

# In this scope it's the dir we are in, in the function scope it will be the
# caller's dir. So, keep our dir in a var.
set(FINDGEM_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR})

function(find_gem GEM_NAME)
    set(GEM_PACKAGE "Gem:${GEM_NAME}")

    configure_file(${FINDGEM_MODULES_DIR}/FindGem.cmake.in Find${GEM_PACKAGE}.cmake @ONLY)

    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}" ${CMAKE_MODULE_PATH})
    find_package(${GEM_PACKAGE} ${ARGN})
endfunction()
