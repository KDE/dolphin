set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)
include(ECMAddTests)

include(FindGem)

find_gem(test-unit REQUIRED)
set_package_properties(Gem:test-unit PROPERTIES
    DESCRIPTION "Ruby gem 'test-unit' required for testing of servicemenu helpers.")

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

# DolphinMainWindowTest
set(dolphinmainwindowtest_SRCS dolphinmainwindowtest.cpp)
qt5_add_resources(dolphinmainwindowtest_SRCS ${CMAKE_SOURCE_DIR}/src/dolphin.qrc)

ecm_add_test(${dolphinmainwindowtest_SRCS}
TEST_NAME dolphinmainwindowtest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)

# DragAndDropHelperTest
ecm_add_test(draganddrophelpertest.cpp LINK_LIBRARIES dolphinprivate Qt5::Test)

# PlacesItemModelTest
if (KF5_VERSION VERSION_GREATER_EQUAL "5.60.0")
    ecm_add_test(placesitemmodeltest.cpp
    TEST_NAME placesitemmodeltest
    LINK_LIBRARIES dolphinprivate dolphinstatic Qt5::Test)
endif()

add_test(NAME servicemenutest
 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../settings/services/test/test_run.rb)
