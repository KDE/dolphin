/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Internet Keywords support (C) 1999 Yves Arrouye <yves@realnames.com>
    Current maintainer Yves Arrouye <yves@realnames.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __main_h__
#define __main_h__ $Id$

#include <klibloader.h>

class KInstance;
class KonqMainView;

class KonqSearcher : public QObject {
    Q_OBJECT
public:
    KonqSearcher( QObject *parent );
    ~KonqSearcher();

protected:
    bool eventFilter( QObject *obj, QEvent *ev );
};

class KonqSearcherFactory : public KLibFactory {
    Q_OBJECT
public:
    KonqSearcherFactory( QObject *parent = 0, const char *name = 0 );
    ~KonqSearcherFactory();

    virtual QObject *create( QObject *parent = 0, const char *name = 0, const char* classname = "QObject" );

    static KInstance *instance();

private:
    static KInstance *s_instance;
};

#endif
