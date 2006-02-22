#! /usr/bin/env bash
#	$EXTRACTRC `find . -name "*.ui"` >> rc.cpp || exit 11
$EXTRACTRC `find . -name "*.rc"` >> rc.cpp || exit 12
$XGETTEXT *.cpp viewer/*.cpp -o $podir/nsplugin.pot
