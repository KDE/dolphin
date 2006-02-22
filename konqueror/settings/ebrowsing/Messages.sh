#! /usr/bin/env bash
$EXTRACTRC plugins/*/*.ui >> rc.cpp || exit 11
$XGETTEXT *.cpp plugins/ikws/*.cpp plugins/shorturi/*.cpp -o $podir/kcmkurifilt.pot
