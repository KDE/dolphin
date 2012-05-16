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

#ifndef PLACESITEMSIGNALHANDLER_H
#define PLACESITEMSIGNALHANDLER_H

#include <QObject>

class PlacesItem;

/**
 * @brief Helper class for PlacesItem to be able to listen to signals
 *        and performing a corresponding action.
 *
 * PlacesItem is derived from KStandardItem, which is no QObject-class
 * on purpose. To be able to internally listening to signals and performing a
 * corresponding action, PlacesItemSignalHandler is used.
 *
 * E.g. if the PlacesItem wants to react on accessibility-changes of a storage-access,
 * the signal-handler can be used like this:
 * <code>
 *     QObject::connect(storageAccess, SIGNAL(accessibilityChanged(bool,QString)),
 *                      signalHandler, SLOT(onAccessibilityChanged()));
 * </code>
 *
 * The slot PlacesItemSignalHandler::onAccessibilityChanged() will call
 * the method PlacesItem::onAccessibilityChanged().
 */
class PlacesItemSignalHandler: public QObject
{
    Q_OBJECT

public:
    explicit PlacesItemSignalHandler(PlacesItem* item, QObject* parent = 0);
    virtual ~PlacesItemSignalHandler();

public slots:
    /**
     * Calls PlacesItem::onAccessibilityChanged()
     */
    void onAccessibilityChanged();

    /**
     * Calls PlacesItem::onTrashDirListerCompleted()
     */
    void onTrashDirListerCompleted();

private:
    PlacesItem* m_item;
};

#endif
