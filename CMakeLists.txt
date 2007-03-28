project(dolphin)

macro_ensure_version( "4.3.0" "${QTVERSION}" QT_GOOD_VERSION )

MESSAGE(STATUS "${QTVERSION} :")
MESSAGE(STATUS "QT_GOOD :<${QT_GOOD_VERSION}>")

if(QT_GOOD_VERSION)
add_subdirectory(src)
endif(QT_GOOD_VERSION)
