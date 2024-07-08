/** Set whether to build Dolphin with support for these technologies or not. */
#cmakedefine01 HAVE_BALOO
#cmakedefine01 HAVE_KUSERFEEDBACK
#cmakedefine01 HAVE_PACKAGEKIT
#cmakedefine01 HAVE_TERMINAL
#cmakedefine01 HAVE_X11

/** The name of the package that needs to be installed so URLs starting with "admin:" can be opened in Dolphin. */
#cmakedefine ADMIN_WORKER_PACKAGE_NAME "@ADMIN_WORKER_PACKAGE_NAME@"

/** The name of the KDE Filelight package. */
#cmakedefine FILELIGHT_PACKAGE_NAME "@FILELIGHT_PACKAGE_NAME@"
