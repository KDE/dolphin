/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    
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
    Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __KURIIKWSFILTER_H__
#define __KURIIKWSFILTER_H__

#include <dcopobject.h>
#include <kgenericfactory.h>

#include <kurifilter.h>

class KInstance;

class KAutoWebSearch : public KURIFilterPlugin, public DCOPObject
{
    K_DCOP
    Q_OBJECT
public:
    KAutoWebSearch(QObject *parent = 0, const char *name = 0,
                   const QStringList &args = QStringList() );
    ~KAutoWebSearch();

    virtual bool filterURI( KURIFilterData& ) const;

k_dcop:
    virtual void configure();
};

#endif

