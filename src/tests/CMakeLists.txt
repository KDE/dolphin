set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)
include(ECMAddTests)

# KItemSetTest
ecm_add_test(kitemsettest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)

# KItemRangeTest
ecm_add_test(kitemrangetest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)


# KItemListSelectionManagerTest
ecm_add_test(kitemlistselectionmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)

# KItemListControllerTest

kconfig_add_kcfg_files(kitemlistcontrollertest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)
ecm_add_test(kitemlistcontrollertest.cpp testdir.cpp ../dolphindebug.cpp ${kitemlistcontrollertest_SRCS}
TEST_NAME kitemlistcontrollertest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# KFileItemListViewTest
kconfig_add_kcfg_files(kfileitemlistviewtest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)
ecm_add_test(kfileitemlistviewtest.cpp testdir.cpp ../dolphindebug.cpp ${kfileitemlistviewtest_SRCS}
TEST_NAME kfileitemlistviewtest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# KFileItemModelTest
kconfig_add_kcfg_files(kfileitemmodeltest_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)
ecm_add_test(kfileitemmodeltest.cpp testdir.cpp ../dolphindebug.cpp ${kfileitemmodeltest_SRCS}
TEST_NAME kfileitemmodeltest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# KFileItemModelBenchmark
kconfig_add_kcfg_files(kfileitemmodelbenchmark_SRCS GENERATE_MOC
    ../settings/dolphin_generalsettings.kcfgc
)
ecm_add_test(kfileitemmodelbenchmark.cpp testdir.cpp ../dolphindebug.cpp ${kfileitemmodelbenchmark_SRCS}
TEST_NAME kfileitemmodelbenchmark
LINK_LIBRARIES  dolphinprivate Qt5::Test)

# KItemListKeyboardSearchManagerTest
ecm_add_test(kitemlistkeyboardsearchmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)

# DolphinSearchBox
if (KF5Baloo_FOUND)
  kconfig_add_kcfg_files(dolphinsearchboxtest_SRCS
      ../search/dolphin_searchsettings.kcfgc
  )
  ecm_add_test(dolphinsearchboxtest.cpp ../search/dolphinfacetswidget.cpp ../search/dolphinsearchbox.cpp ${dolphinsearchboxtest_SRCS}
  TEST_NAME dolphinsearchboxtest
  LINK_LIBRARIES dolphinprivate Qt5::Test)
endif()

# KStandardItemModelTest
ecm_add_test(kstandarditemmodeltest.cpp ../dolphindebug.cpp
TEST_NAME kstandarditemmodeltest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# ViewPropertiesTest
kconfig_add_kcfg_files(viewpropertiestest_SRCS GENERATE_MOC
  ../settings/dolphin_generalsettings.kcfgc
  ../settings/dolphin_directoryviewpropertysettings.kcfgc
)
ecm_add_test(viewpropertiestest.cpp testdir.cpp ../dolphindebug.cpp ${viewpropertiestest_SRCS}
TEST_NAME viewpropertiestest
LINK_LIBRARIES dolphinprivate Qt5::Test)
