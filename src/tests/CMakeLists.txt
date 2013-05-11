set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BUILD_DIR}/.. ${KDE4_INCLUDES} )

# needed on windows to correctly use the files from dolphinprivate
add_definitions(-DLIBDOLPHINPRIVATE_EXPORT=)
# KItemListSelectionManagerTest
set(kitemlistselectionmanagertest_SRCS
    kitemlistselectionmanagertest.cpp
    ../kitemviews/kitemlistselectionmanager.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_unit_test(kitemlistselectionmanagertest TEST ${kitemlistselectionmanagertest_SRCS})
target_link_libraries(kitemlistselectionmanagertest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# KItemListControllerTest
set(kitemlistcontrollertest_SRCS
    kitemlistcontrollertest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
    ../kitemviews/kitemlistcontainer.cpp
    ../kitemviews/kitemlistwidget.cpp
    ../kitemviews/kitemlistviewaccessible.cpp
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
)
kde4_add_unit_test(kitemlistcontrollertest TEST ${kitemlistcontrollertest_SRCS})
target_link_libraries(kitemlistcontrollertest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

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
    ../kitemviews/kstandarditemlistview.cpp
    ../kitemviews/kstandarditemlistwidget.cpp
)
kde4_add_unit_test(kfileitemlistviewtest TEST ${kfileitemlistviewtest_SRCS})
target_link_libraries(kfileitemlistviewtest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# KFileItemModelTest
set(kfileitemmodeltest_SRCS
    kfileitemmodeltest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_unit_test(kfileitemmodeltest TEST ${kfileitemmodeltest_SRCS})
target_link_libraries(kfileitemmodeltest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# KFileItemModelBenchmark
set(kfileitemmodelbenchmark_SRCS
    kfileitemmodelbenchmark.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_executable(kfileitemmodelbenchmark TEST ${kfileitemmodelbenchmark_SRCS})
target_link_libraries(kfileitemmodelbenchmark dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# KItemListKeyboardSearchManagerTest
set(kitemlistkeyboardsearchmanagertest_SRCS
    kitemlistkeyboardsearchmanagertest.cpp
    ../kitemviews/private/kitemlistkeyboardsearchmanager.cpp
)
kde4_add_unit_test(kitemlistkeyboardsearchmanagertest TEST ${kitemlistkeyboardsearchmanagertest_SRCS})
target_link_libraries(kitemlistkeyboardsearchmanagertest ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# DolphinSearchBox
if (Nepomuk_FOUND)
  set(dolphinsearchboxtest_SRCS
      dolphinsearchboxtest.cpp
      ../search/dolphinfacetswidget.cpp
      ../search/dolphinsearchbox.cpp
      ../search/dolphinsearchinformation.cpp
  )
  kde4_add_kcfg_files(dolphinsearchboxtest_SRCS
      ../search/dolphin_searchsettings.kcfgc
  )
  kde4_add_unit_test(dolphinsearchboxtest TEST ${dolphinsearchboxtest_SRCS})
  target_link_libraries(dolphinsearchboxtest ${KDE4_KIO_LIBS} ${SOPRANO_LIBRARIES} ${NEPOMUK_CORE_LIBRARY}  nepomukutils ${QT_QTTEST_LIBRARY})
endif (Nepomuk_FOUND)

# KStandardItemModelTest
set(kstandarditemmodeltest_SRCS
    kstandarditemmodeltest.cpp
    ../kitemviews/kstandarditem.cpp
    ../kitemviews/kstandarditemmodel.cpp
    ../kitemviews/kitemmodelbase.cpp
)
kde4_add_unit_test(kstandarditemmodeltest TEST ${kstandarditemmodeltest_SRCS})
target_link_libraries(kstandarditemmodeltest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# ViewPropertiesTest
set(viewpropertiestest_SRCS
    viewpropertiestest.cpp
    testdir.cpp
    ../views/viewproperties.cpp
)
kde4_add_kcfg_files(viewpropertiestest_SRCS
  ../settings/dolphin_generalsettings.kcfgc
  ../settings/dolphin_directoryviewpropertysettings.kcfgc
)
kde4_add_unit_test(viewpropertiestest TEST ${viewpropertiestest_SRCS})
target_link_libraries(viewpropertiestest dolphinprivate ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
