set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt6Test CONFIG REQUIRED)
include(ECMAddTests)

include(FindGem)  # For servicemenutest, see bottom of this file

# KItemSetTest
ecm_add_test(kitemsettest.cpp LINK_LIBRARIES dolphinprivate Qt6::Test)

# KItemRangeTest
ecm_add_test(kitemrangetest.cpp LINK_LIBRARIES dolphinprivate Qt6::Test)


# KItemListSelectionManagerTest
ecm_add_test(kitemlistselectionmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt6::Test)

# KItemListControllerTest
ecm_add_test(kitemlistcontrollertest.cpp testdir.cpp
TEST_NAME kitemlistcontrollertest
LINK_LIBRARIES dolphinprivate Qt6::Test)

# KFileItemListViewTest
ecm_add_test(kfileitemlistviewtest.cpp testdir.cpp
TEST_NAME kfileitemlistviewtest
LINK_LIBRARIES dolphinprivate Qt6::Test)

# KFileItemModelTest
ecm_add_test(kfileitemmodeltest.cpp testdir.cpp
TEST_NAME kfileitemmodeltest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)

# KFileItemModelBenchmark, not run automatically with `ctest` or `make test`
add_executable(kfileitemmodelbenchmark kfileitemmodelbenchmark.cpp testdir.cpp)
target_link_libraries(kfileitemmodelbenchmark dolphinprivate Qt6::Test)

# KItemListKeyboardSearchManagerTest
ecm_add_test(kitemlistkeyboardsearchmanagertest.cpp LINK_LIBRARIES dolphinprivate Qt6::Test)

# DolphinSearchBox
if (KF5Baloo_FOUND)
    ecm_add_test(dolphinsearchboxtest.cpp
    TEST_NAME dolphinsearchboxtest
    LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)
endif()

# DolphinQuery
if (KF5Baloo_FOUND)
    ecm_add_test(dolphinquerytest.cpp
    TEST_NAME dolphinquerytest
    LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)
endif()

# ViewPropertiesTest
ecm_add_test(viewpropertiestest.cpp testdir.cpp
TEST_NAME viewpropertiestest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)

# DolphinMainWindowTest
ecm_add_test(dolphinmainwindowtest.cpp testdir.cpp ${CMAKE_SOURCE_DIR}/src/dolphin.qrc
TEST_NAME dolphinmainwindowtest
LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)

# DragAndDropHelperTest
ecm_add_test(draganddrophelpertest.cpp LINK_LIBRARIES dolphinprivate Qt6::Test)

find_gem(test-unit)
set_package_properties(Gem:test-unit PROPERTIES
    TYPE RECOMMENDED
    DESCRIPTION "Ruby gem 'test-unit' required for testing of servicemenu helpers.")
if (Gem:test-unit_FOUND)
    add_test(NAME servicemenutest
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../settings/contextmenu/test/test_run.rb)
endif()
