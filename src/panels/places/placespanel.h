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

#include <QUrl>
#include <QMimeData>
#include <panels/panel.h>

class KItemListController;
class PlacesItem;
class PlacesItemModel;
class PlacesView;
class QGraphicsSceneDragDropEvent;
class KJob;
/**
 * @brief Combines bookmarks and mounted devices as list.
 */
class PlacesPanel : public Panel
{
    Q_OBJECT

public:
    explicit PlacesPanel(QWidget* parent);
    ~PlacesPanel() override;
    void proceedWithTearDown();

signals:
    void placeActivated(const QUrl& url);
    void placeMiddleClicked(const QUrl& url);
    void errorMessage(const QString& error);
    void storageTearDownRequested(const QString& mountPath);
    void storageTearDownExternallyRequested(const QString& mountPath);

protected:
    bool urlChanged() override;
    void showEvent(QShowEvent* event) override;

public slots:
    void readSettings() override;

private slots:
    void slotItemActivated(int index);
    void slotItemMiddleClicked(int index);
    void slotItemContextMenuRequested(int index, const QPointF& pos);
    void slotViewContextMenuRequested(const QPointF& pos);
    void slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotItemDropEventStorageSetupDone(int index, bool success);
    void slotAboveItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent);
    void slotTrashUpdated(KJob* job);
    void slotStorageSetupDone(int index, bool success);

private:
    void emptyTrash();
    void addEntry();
    void editEntry(int index);

    /**
     * Selects the item that has the closest URL for the URL set
     * for the panel (see Panel::setUrl()).
     */
    void selectClosestItem();

    void triggerItem(int index, Qt::MouseButton button);

private:
    KItemListController* m_controller;
    PlacesItemModel* m_model;
    PlacesView* m_view;

    QUrl m_storageSetupFailedUrl;
    Qt::MouseButton m_triggerStorageSetupButton;

    int m_itemDropEventIndex;
    QMimeData* m_itemDropEventMimeData;
    QDropEvent* m_itemDropEvent;
};

#endif // PLACESPANEL_H
