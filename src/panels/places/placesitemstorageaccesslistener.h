/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef PLACESITEMSTORAGEACCESSLISTENER_H
#define PLACESITEMSTORAGEACCESSLISTENER_H

#include <QObject>

class PlacesItem;

/**
 * @brief Helper class for PlacesItem to listen to accessibility changes
 *        of the storage access.
 *
 * Connects to the storage access from the given places item and
 * calls PlacesItem::onAccessibilityChanged() in case if the accessibility
 * has been changed.
 */
class PlacesItemStorageAccessListener: public QObject
{
    Q_OBJECT

public:
    explicit PlacesItemStorageAccessListener(PlacesItem* item, QObject* parent = 0);
    virtual ~PlacesItemStorageAccessListener();

private slots:
    void slotAccessibilityChanged();

private:
    PlacesItem* m_item;
};

#endif
