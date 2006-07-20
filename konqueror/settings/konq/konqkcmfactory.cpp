/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "konqkcmfactory.h"
#include <kinstance.h>
#include <QDesktopWidget>
#include <kapplication.h>

static KInstance *_kcmkonq = 0;

KInstance* _globalInstance()
{
    if (_kcmkonq)
        return _kcmkonq;
    _kcmkonq = new KInstance("kcmkonq");
    return _kcmkonq;
}

QString _desktopConfigName()
{
    int desktop = KApplication::desktop()->primaryScreen();
    QString name;
    if (desktop == 0)
        name = "kdesktoprc";
    else
        name.sprintf("kdesktop-screen-%drc", desktop);

    return name;
}

// vim: sw=4 ts=4 noet
