#! /usr/bin/env bash
### TEMPORARY for KDE4, might extract too much
$EXTRACTRC `find . -name \*.rc` >> rc.cpp || exit 11
$EXTRACTRC sidebar/trees/history_module/history_dlg.ui >> rc.cpp || exit 12
$XGETTEXT -kaliasLocal `find . -name \*.cc -o -name \*.cpp -o -name \*.h` -o $podir/konqueror.pot
