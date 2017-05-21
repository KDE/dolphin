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
ecm_add_test(kitemlistcontrollertest.cpp testdir.cpp
TEST_NAME kitemlistcontrollertest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# KFileItemListViewTest
ecm_add_test(kfileitemlistviewtest.cpp testdir.cpp
TEST_NAME kfileitemlistviewtest
LINK_LIBRARIES dolphinprivate Qt5::Test)

# KFileItemModelTest
ecm_add_test(kfileitemmodeltest.cpp testdir.cpp
TEST_NAME kfileitemmodeltest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)

# KFileItemModelBenchmark
ecm_add_test(kfileitemmodelbenchmark.cpp testdir.cpp
TEST_NAME kfileitemmodelbenchmark
LINK_LIBRARIES  dolphinprivate Qt5::Test)

# KItemListKeyboardSearchManagerTest
ecm_add_test(kitemlistkeyboardsearchmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)

# DolphinSearchBox
if (KF5Baloo_FOUND)
  ecm_add_test(dolphinsearchboxtest.cpp
  TEST_NAME dolphinsearchboxtest
  LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)
endif()

# KStandardItemModelTest
ecm_add_test(kstandarditemmodeltest.cpp
TEST_NAME kstandarditemmodeltest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)

# ViewPropertiesTest
ecm_add_test(viewpropertiestest.cpp testdir.cpp
TEST_NAME viewpropertiestest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)
