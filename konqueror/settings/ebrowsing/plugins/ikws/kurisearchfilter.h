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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __KURISEARCHFILTER_H__
#define __KURISEARCHFILTER_H__

#include <dcopobject.h>
#include <kgenericfactory.h>

#include <kurifilter.h>

class KInstance;

class KURISearchFilter : public KURIFilterPlugin, public DCOPObject
{
    Q_OBJECT
    K_DCOP
public:
    KURISearchFilter(QObject *parent = 0, const QStringList &args = QStringList() );
    ~KURISearchFilter();

    virtual bool filterURI( KURIFilterData& ) const;
    virtual KCModule *configModule(QWidget *parent = 0, const char *name = 0) const;
    virtual QString configName() const;

k_dcop:
    virtual void configure();
};

#endif

