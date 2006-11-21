-----------------------------------------------
Kde application framework template quickstart
Author: Thomas Nagy
Date: 2004-03-22
-----------------------------------------------

This README file explains you basic things for starting with
this application template.


** Building and installing **

* Build the configure script by "make -f Makefile.cvs"

* To clean, use "make clean", and to clean everything 
(remove the makefiles, etc), use "make distclean"

* To distribute your program, try "make dist".
This will make a compact tarball archive of your release with the
necessary scripts inside.

* Modifying the auto-tools scripts
for automake scripts there is an excellent tutorial there :
http://developer.kde.org/documentation/other/makefile_am_howto.html

* Simplify your life : install the project in your home directory for
testing purposes.
./configure --prefix=/home/user/dummyfolder/
In the end when you finished the development you can
rm -rf /home/user/dummyfolder/
without fear.


** Technologies **

* Build the menus of your application easily
kde applications now use an xml file (*ui.rc file) to build the menus.
This allow a great customization of the application. However, when
programming the menu is shown only after a "make install"

For more details, consult :
http://devel-home.kde.org/~larrosa/tutorial/p9.html
http://developer.kde.org/documentation/tutorials/xmlui/preface.html

* Use KConfig XT to create your configuration dialogs and make 
them more maintainable.

For more details, consult :
http://developer.kde.org/documentation/tutorials/kconfigxt/kconfigxt.html

* With KParts, you can embed other kde components in your program, or make your program
embeddable in other apps. For example, the kmplayer kpart can be called to play videos
in your app.

For more details, consult :
http://www-106.ibm.com/developerworks/library/l-kparts/
http://developer.kde.org/documentation/tutorials/dot/writing-plugins.html
http://developer.kde.org/documentation/tutorials/developing-a-plugin-structure/index.html

* With dcop, you can control your app from other applications
Make sure to include K_DCOP and a kdcop: section in your .h file
http://developer.kde.org/documentation/tutorials/dot/dcopiface/dcop-interface.html


** Documentation **

* For the translations :
1. Download a patched gettext which can be found at:
 http://public.kde.planetmirror.com/pub/kde/devel/gettext-kde/
2. Install that gettext in ~/bin/
3. cd ~/yourproject, export PATH=~/bin:$PATH, export
KDEDIR=/where_your_KDE3_is
4. make -f admin/Makefile.common package-messages
5. make package-messages
6. Translate the po files (not the pot!!) with kbabel or xemacs

* Do not forget to write the documentation for your kde app 
edit the documentation template index.docbook in doc/



