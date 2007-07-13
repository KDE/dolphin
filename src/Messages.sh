#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -name \*.kcfg` > rc.cpp
$XGETTEXT *.cpp -o $podir/dolphin.pot

