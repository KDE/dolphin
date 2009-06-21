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

#ifndef KTOOLTIPITEM_H
#define KTOOLTIPITEM_H

#include <QVariant>

class QString;
class QIcon;
class QSize;
class QPainter;
class QRegion;

class KToolTipItemPrivate;

/**
 * KToolTipItem contains the data to be displayed in a tooltip.
 *
 * Custom data can be stored as QVariants in the object by calling
 * setData() with a custom item role, and retrieved and displayed
 * by a tooltip delegate by calling data().
 *
 * The default tooltip delegate uses Qt::DecorationRole and
 * Qt::DisplayRole.
 *
 * To display the tooltip, call KToolTip::showTip() with a pointer
 * to the KToolTipItem.
 *
 * You can reimplement the setData() and/or data() methods in this
 * class to implement on-demand loading of data.
 */
class KToolTipItem
{
public:
     enum ItemType { DefaultType, UserType = 1000 };

     /**
      * Creates a KToolTipItem with @p text and no icon.
      */
     explicit KToolTipItem(const QString &text, int type = DefaultType);

     /**
      * Creates a KToolTipItem with an @p icon and @p text.
      */
     KToolTipItem(const QIcon &icon, const QString &text, int type = DefaultType);

     /**
      * Destroys the KToolTipItem.
      */
     virtual ~KToolTipItem();

     /**
      * Returns the item type.
      */
     int type() const;

     QString text() const;
     QIcon icon() const;

     virtual QVariant data(int role) const;
     virtual void setData(int role, const QVariant &data);

private:
     KToolTipItemPrivate * const d;
};

#endif
