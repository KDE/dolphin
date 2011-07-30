set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BUILD_DIR}/.. ${KDE4_INCLUDES} )

# KFileItemListViewTest
set(kfileitemlistviewtest_SRCS
    kfileitemlistviewtest.cpp
    testdir.cpp
    ../kitemviews/kfileitemmodel.cpp
    ../kitemviews/kfileitemlistview.cpp
    ../kitemviews/kitemmodelbase.cpp
    ../kitemviews/kitemlistview.cpp
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

# DolphinSearchBox
if (Nepomuk_FOUND)
  set(dolphinsearchboxtest_SRCS
      dolphinsearchboxtest.cpp
      ../search/dolphinsearchbox.cpp
      ../search/dolphinsearchinformation.cpp
  )
  kde4_add_kcfg_files(dolphinsearchboxtest_SRCS
      ../search/dolphin_searchsettings.kcfgc
  )
  kde4_add_unit_test(dolphinsearchboxtest TEST ${dolphinsearchboxtest_SRCS})
  target_link_libraries(dolphinsearchboxtest ${KDE4_KIO_LIBS} ${SOPRANO_LIBRARIES} ${NEPOMUK_LIBRARIES} ${NEPOMUK_QUERY_LIBRARIES} nepomukutils ${QT_QTTEST_LIBRARY})
endif (Nepomuk_FOUND)
