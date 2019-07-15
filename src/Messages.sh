#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp \! -path '*/servicemenuinstaller/*'` -o $podir/dolphin.pot
rm -f rc.cpp
