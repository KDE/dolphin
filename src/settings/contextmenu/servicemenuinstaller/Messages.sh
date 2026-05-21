#! /usr/bin/env bash

# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-FileCopyrightText: 2020 Duong Do Minh Chau <duongdominhchau@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

$XGETTEXT `find . -name \*.cpp` -o $podir/dolphin_servicemenuinstaller.pot
