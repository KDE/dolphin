set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt${QT_MAJOR_VERSION}Test CONFIG REQUIRED)
include(ECMAddTests)

include(FindGem)  # For servicemenutest, see bottom of this file

# KItemSetTest
ecm_add_test(kitemsettest.cpp LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KItemRangeTest
ecm_add_test(kitemrangetest.cpp LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)


# KItemListSelectionManagerTest
ecm_add_test(kitemlistselectionmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KItemListControllerTest
ecm_add_test(kitemlistcontrollertest.cpp testdir.cpp
TEST_NAME kitemlistcontrollertest
LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KItemListControllerExpandTest
ecm_add_test(kitemlistcontrollerexpandtest.cpp testdir.cpp
TEST_NAME kitemlistcontrollerexpandtest
LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KFileItemListViewTest
ecm_add_test(kfileitemlistviewtest.cpp testdir.cpp
TEST_NAME kfileitemlistviewtest
LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KFileItemModelTest
ecm_add_test(kfileitemmodeltest.cpp testdir.cpp
TEST_NAME kfileitemmodeltest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt${QT_MAJOR_VERSION}::Test)

# KFileItemModelBenchmark, not run automatically with `ctest` or `make test`
add_executable(kfileitemmodelbenchmark kfileitemmodelbenchmark.cpp testdir.cpp)
target_link_libraries(kfileitemmodelbenchmark dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# KItemListKeyboardSearchManagerTest
ecm_add_test(kitemlistkeyboardsearchmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

# DolphinSearchBox
if (KF5Baloo_FOUND)
    ecm_add_test(dolphinsearchboxtest.cpp
    TEST_NAME dolphinsearchboxtest
    LINK_LIBRARIES dolphinprivate dolphinstatic Qt${QT_MAJOR_VERSION}::Test)
endif()

# DolphinQuery
if (KF5Baloo_FOUND)
    ecm_add_test(dolphinquerytest.cpp
    TEST_NAME dolphinquerytest
    LINK_LIBRARIES dolphinprivate dolphinstatic Qt${QT_MAJOR_VERSION}::Test)
endif()

# ViewPropertiesTest
ecm_add_test(viewpropertiestest.cpp testdir.cpp
TEST_NAME viewpropertiestest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt${QT_MAJOR_VERSION}::Test)

# DolphinMainWindowTest
ecm_add_test(dolphinmainwindowtest.cpp testdir.cpp ${CMAKE_SOURCE_DIR}/src/dolphin.qrc
TEST_NAME dolphinmainwindowtest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt${QT_MAJOR_VERSION}::Test)

# DragAndDropHelperTest
ecm_add_test(draganddrophelpertest.cpp LINK_LIBRARIES dolphinprivate Qt${QT_MAJOR_VERSION}::Test)

find_gem(test-unit)
set_package_properties(Gem:test-unit PROPERTIES
    TYPE RECOMMENDED
    DESCRIPTION "Ruby gem 'test-unit' required for testing of servicemenu helpers.")
if (Gem:test-unit_FOUND)
    add_test(NAME servicemenutest
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../settings/contextmenu/test/test_run.rb)
endif()
