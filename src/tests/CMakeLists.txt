set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)
include(ECMMarkAsTest)

# needed on windows to correctly use the files from dolphinprivate
add_definitions(-DLIBDOLPHINPRIVATE_EXPORT=)

# KItemSetTest
set(kitemsettest_SRCS
    kitemsettest.cpp
    ../kitemviews/kitemset.cpp
)

add_executable(kitemsettest ${kitemsettest_SRCS})
add_test(kitemsettest kitemsettest)
ecm_mark_as_test(kitemsettest)
target_link_libraries(kitemsettest dolphinprivate Qt5::Test)

# KItemRangeTest
set(kitemrangetest_SRCS
    kitemrangetest.cpp
)

add_executable(kitemrangetest ${kitemrangetest_SRCS})
add_test(kitemrangetest kitemrangetest)
ecm_mark_as_test(kitemrangetest)
target_link_libraries(kitemrangetest dolphinprivate Qt5::Test)


# KItemListSelectionManagerTest
set(kitemlistselectionmanagertest_SRCS
    kitemlistselectionmanagertest.cpp
    ../kitemviews/kitemlistselectionmanager.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemset.cpp
)

add_executable(kitemlistselectionmanagertest ${kitemlistselectionmanagertest_SRCS})
add_test(kitemlistselectionmanagertest kitemlistselectionmanagertest)
ecm_mark_as_test(kitemlistselectionmanagertest)
target_link_libraries(kitemlistselectionmanagertest dolphinprivate Qt5::Test)

# KItemListControllerTest
set(kitemlistcontrollertest_SRCS
    kitemlistcontrollertest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kfileitemlistwidget.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
    ../kitemviews/kitemlistcontainer.cpp
    ../kitemviews/kitemlistwidget.cpp
    ../kitemviews/kitemlistviewaccessible.cpp
    ../kitemviews/kitemset.cpp
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
    ../dolphindebug.cpp
)

kconfig_add_kcfg_files(kitemlistcontrollertest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)

add_executable(kitemlistcontrollertest ${kitemlistcontrollertest_SRCS})
add_test(kitemlistcontrollertest kitemlistcontrollertest)
ecm_mark_as_test(kitemlistcontrollertest)
target_link_libraries(kitemlistcontrollertest dolphinprivate Qt5::Test)

# KFileItemListViewTest
set(kfileitemlistviewtest_SRCS
    kfileitemlistviewtest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
    ../kitemviews/kitemlistviewaccessible.cpp
    ../kitemviews/kitemlistcontainer.cpp
    ../kitemviews/kitemlistwidget.cpp
    ../kitemviews/kitemset.cpp
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
    ../dolphindebug.cpp
)

kconfig_add_kcfg_files(kfileitemlistviewtest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)

add_executable(kfileitemlistviewtest ${kfileitemlistviewtest_SRCS})
add_test(kfileitemlistviewtest kfileitemlistviewtest)
ecm_mark_as_test(kfileitemlistviewtest)
target_link_libraries(kfileitemlistviewtest dolphinprivate Qt5::Test)

# KFileItemModelTest
set(kfileitemmodeltest_SRCS
    kfileitemmodeltest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemset.cpp
    ../dolphindebug.cpp
)

kconfig_add_kcfg_files(kfileitemmodeltest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)

add_executable(kfileitemmodeltest ${kfileitemmodeltest_SRCS})
add_test(kfileitemmodeltest kfileitemmodeltest)
ecm_mark_as_test(kfileitemmodeltest)
target_link_libraries(kfileitemmodeltest dolphinprivate Qt5::Test)

# KFileItemModelBenchmark
set(kfileitemmodelbenchmark_SRCS
    kfileitemmodelbenchmark.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../dolphindebug.cpp
)

kconfig_add_kcfg_files(kfileitemmodelbenchmark_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)

add_executable(kfileitemmodelbenchmark ${kfileitemmodelbenchmark_SRCS})
ecm_mark_as_test(kfileitemmodelbenchmark)
target_link_libraries(kfileitemmodelbenchmark dolphinprivate Qt5::Test)

# KItemListKeyboardSearchManagerTest
set(kitemlistkeyboardsearchmanagertest_SRCS
    kitemlistkeyboardsearchmanagertest.cpp
    ../kitemviews/private/kitemlistkeyboardsearchmanager.cpp
)

add_executable(kitemlistkeyboardsearchmanagertest ${kitemlistkeyboardsearchmanagertest_SRCS})
add_test(kitemlistkeyboardsearchmanagertest kitemlistkeyboardsearchmanagertest)
ecm_mark_as_test(kitemlistkeyboardsearchmanagertest)
target_link_libraries(kitemlistkeyboardsearchmanagertest dolphinprivate Qt5::Test)

# DolphinSearchBox
if (KF5Baloo_FOUND)
  set(dolphinsearchboxtest_SRCS
      dolphinsearchboxtest.cpp
      ../search/dolphinfacetswidget.cpp
      ../search/dolphinsearchbox.cpp
  )
  kconfig_add_kcfg_files(dolphinsearchboxtest_SRCS
      ../search/dolphin_searchsettings.kcfgc
  )

  add_executable(dolphinsearchboxtest ${dolphinsearchboxtest_SRCS})
  add_test(dolphinsearchboxtest dolphinsearchboxtest)
  ecm_mark_as_test(dolphinsearchboxtest)
  target_link_libraries(dolphinsearchboxtest dolphinprivate Qt5::Test)
endif()

# KStandardItemModelTest
set(kstandarditemmodeltest_SRCS
    kstandarditemmodeltest.cpp
    ../kitemviews/kstandarditem.cpp
    ../kitemviews/kstandarditemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../dolphindebug.cpp
)

add_executable(kstandarditemmodeltest ${kstandarditemmodeltest_SRCS})
add_test(kstandarditemmodeltest kstandarditemmodeltest)
ecm_mark_as_test(kstandarditemmodeltest)
target_link_libraries(kstandarditemmodeltest dolphinprivate Qt5::Test)

# ViewPropertiesTest
set(viewpropertiestest_SRCS
    viewpropertiestest.cpp
    testdir.cpp
    ../views/viewproperties.cpp
    ../dolphindebug.cpp
)
kconfig_add_kcfg_files(viewpropertiestest_SRCS GENERATE_MOC
  ../settings/dolphin_generalsettings.kcfgc
  ../settings/dolphin_directoryviewpropertysettings.kcfgc
)

add_executable(viewpropertiestest ${viewpropertiestest_SRCS})
add_test(viewpropertiestest viewpropertiestest)
ecm_mark_as_test(viewpropertiestest)
target_link_libraries(viewpropertiestest dolphinprivate Qt5::Test)
