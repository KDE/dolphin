#! /usr/bin/env bash

# SPDX-FileCopyrightText: 2006, 2010 Laurent Montel <montel@kde.org>
# SPDX-FileCopyrightText: 2007 Albert Astals Cid <tsdgeos@terra.es>
# SPDX-FileCopyrightText: 2007 Stephan Kulow <coolo@kde.org>
# SPDX-FileCopyrightText: 2007, 2008 Chusslove Illich <caslav.ilic@gmx.net>
# SPDX-FileCopyrightText: 2019 Pino Toscano <pino@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp \! -path '*/servicemenuinstaller/*'` -o $podir/dolphin.pot
rm -f rc.cpp
