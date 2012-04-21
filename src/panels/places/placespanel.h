/***************************************************************************
 *   Copyright (C) 2008-2012 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2010 by Christian Muehlhaeuser <muesli@gmail.com>       *
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

#ifndef PLACESPANEL_H
#define PLACESPANEL_H

#include <panels/panel.h>
#include <QSet>

class KBookmarkManager;
class KItemListController;
class KStandardItemModel;

/**
 * @brief Combines bookmarks and mounted devices as list.
 */
class PlacesPanel : public Panel
{
    Q_OBJECT

public:
    PlacesPanel(QWidget* parent);
    virtual ~PlacesPanel();

signals:
    void placeActivated(const KUrl& url);
    void placeMiddleClicked(const KUrl& url);

protected:
    virtual bool urlChanged();
    virtual void showEvent(QShowEvent* event);

private slots:
    void slotItemActivated(int index);
    void slotItemMiddleClicked(int index);
    void slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent);

private:
    void loadBookmarks();

private:
    KItemListController* m_controller;
    KStandardItemModel* m_model;
    QSet<QString> m_availableDevices;
    KBookmarkManager* m_bookmarkManager;
};

#endif // PLACESPANEL_H
