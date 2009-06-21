/***************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "ktooltipitem.h"
#include "ktooltip_p.h"

#include <QIcon>

class KToolTipItemPrivate
{
public:
    QMap<int, QVariant> map;
    int type;
};

KToolTipItem::KToolTipItem(const QString &text, int type)
    : d(new KToolTipItemPrivate)
{
    d->map[Qt::DisplayRole] = text;
    d->type = type;
}

KToolTipItem::KToolTipItem(const QIcon &icon, const QString &text, int type)
    : d(new KToolTipItemPrivate)
{
    d->map[Qt::DecorationRole] = icon;
    d->map[Qt::DisplayRole]    = text;
    d->type = type;
}

KToolTipItem::~KToolTipItem()
{
    delete d;
}

int KToolTipItem::type() const
{
    return d->type;
}

QString KToolTipItem::text() const
{
    return data(Qt::DisplayRole).toString();
}

QIcon KToolTipItem::icon() const
{
    return qvariant_cast<QIcon>(data(Qt::DecorationRole));
}

QVariant KToolTipItem::data(int role) const
{
    return d->map.value(role);
}

void KToolTipItem::setData(int role, const QVariant &data)
{
    d->map[role] = data;
    KToolTipManager::instance()->update();
}
