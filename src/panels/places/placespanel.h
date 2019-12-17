/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2010 by Christian Muehlhaeuser <muesli@gmail.com>       *
 *   Copyright (C) 2019 by Kai Uwe Broulik <kde@broulik.de>                *
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

#pragma once

#include <QList>
#include <QPersistentModelIndex>

#include <KFilePlacesView>

class QAction;
class QTimer;

class PlacesPanel : public KFilePlacesView
{
    Q_OBJECT

public:
    PlacesPanel(QWidget* parent);
    ~PlacesPanel() override;

    void setUrl(const QUrl &url); // shadows KFilePlacesView::setUrl

    void readSettings();

    QList<QAction*> customContextMenuActions() const;
    void setCustomContextMenuActions(const QList<QAction*>& actions);

    int hiddenItemsCount() const;

    void proceedWithTearDown();

signals:
    // forwarded from KFilePlacesModel
    void errorMessage(const QString &errorMessage);

    void storageTearDownRequested(const QString& mountPath);
    void storageTearDownExternallyRequested(const QString& mountPath);

protected:
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

private slots:
    void slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent);
    void slotDragActivationTimeout();
    void slotContextMenuAboutToShow(const QModelIndex &index, QMenu *menu);
    void slotTeardownRequested(const QModelIndex &index);
    void slotTrashEmptinessChanged(bool isEmpty);

private:
    void slotRowsInserted(const QModelIndex &parent, int first, int last);
    void slotRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

    void connectDeviceSignals(const QModelIndex &index);

    QUrl m_url;

    QList<QAction *> m_customContextMenuActions;

    QTimer *m_dragActivationTimer = nullptr;
    QPersistentModelIndex m_pendingDragActivation;

    QPersistentModelIndex m_indexToTeardown;
};
