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

#include "placespanel.h"

#include "dolphin_generalsettings.h"
#include "dolphinplacesmodelsingleton.h"
#include "views/draganddrophelper.h"

#include <QMenu>
#include <QTimer>

#include <KFilePlacesModel>
#include <KIO/DropJob>

#include <Solid/Device>
#include <Solid/StorageAccess>

PlacesPanel::PlacesPanel(QWidget* parent) :
    KFilePlacesView(parent)
{
    setDropOnPlaceEnabled(true);

    connect(this, &KFilePlacesView::urlsDropped,
            this, &PlacesPanel::slotUrlsDropped);

    connect(this, &KFilePlacesView::contextMenuAboutToShow,
            this, &PlacesPanel::slotContextMenuAboutToShow);

    // Replace default teardown handling with our own
    disconnect(this, &KFilePlacesView::teardownRequested, this, nullptr);
    connect(this, &KFilePlacesView::teardownRequested, this, &PlacesPanel::slotTeardownRequested);
}

PlacesPanel::~PlacesPanel() = default;

void PlacesPanel::readSettings()
{
    if (GeneralSettings::autoExpandFolders()) {
        if (!m_dragActivationTimer) {
            m_dragActivationTimer = new QTimer(this);
            m_dragActivationTimer->setInterval(750);
            m_dragActivationTimer->setSingleShot(true);
            connect(m_dragActivationTimer, &QTimer::timeout,
                    this, &PlacesPanel::slotDragActivationTimeout);
        }
    } else {
        delete m_dragActivationTimer;
        m_dragActivationTimer = nullptr;
        m_pendingDragActivation = QPersistentModelIndex();
    }
}

QList<QAction *> PlacesPanel::customContextMenuActions() const
{
    return m_customContextMenuActions;
}

void PlacesPanel::setCustomContextMenuActions(const QList<QAction *> &actions)
{
    m_customContextMenuActions = actions;
}

int PlacesPanel::hiddenItemsCount() const
{
    return DolphinPlacesModelSingleton::instance().placesModel()->hiddenCount();
}

void PlacesPanel::proceedWithTearDown()
{
    if (m_indexToTeardown.isValid()) {
        static_cast<KFilePlacesModel *>(model())->requestTeardown(m_indexToTeardown);
    }
}

void PlacesPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        KFilePlacesView::showEvent(event);
        return;
    }

    if (!model()) {
        readSettings();

        auto *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
        setModel(placesModel);
        connect(placesModel, &KFilePlacesModel::errorMessage, this, &PlacesPanel::errorMessage);

        connect(placesModel, &QAbstractItemModel::rowsInserted, this, &PlacesPanel::slotRowsInserted);
        connect(placesModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &PlacesPanel::slotRowsAboutToBeRemoved);

        for (int i = 0; i < model()->rowCount(); ++i) {
            connectDeviceSignals(model()->index(i, 0, QModelIndex()));
        }
    }

    KFilePlacesView::showEvent(event);
}

void PlacesPanel::dragEnterEvent(QDragEnterEvent *event)
{
    KFilePlacesView::dragEnterEvent(event);
}

void PlacesPanel::dragMoveEvent(QDragMoveEvent *event)
{
    KFilePlacesView::dragMoveEvent(event);

    if (!m_dragActivationTimer) {
        return;
    }

    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        return;
    }

    // TODO should we ignore our own drags? Dolphin doesn't seem to
    // at least ignore when we drag the same index onto us?
    QPersistentModelIndex persistentIndex(index);
    if (!m_pendingDragActivation.isValid() || m_pendingDragActivation != persistentIndex) {
        m_pendingDragActivation = persistentIndex;
        m_dragActivationTimer->start();
    }
}

void PlacesPanel::dragLeaveEvent(QDragLeaveEvent *event)
{
    KFilePlacesView::dragLeaveEvent(event);

    if (m_dragActivationTimer) {
        m_dragActivationTimer->stop();
        m_pendingDragActivation = QPersistentModelIndex();
    }
}

void PlacesPanel::slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent)
{
    KIO::DropJob *job = DragAndDropHelper::dropUrls(dest, event, parent);
    if (job) {
        connect(job, &KIO::DropJob::result, this, [this](KJob *job) {
            if (job->error()) {
                emit errorMessage(job->errorString());
            }
        });
    }
}

void PlacesPanel::slotDragActivationTimeout()
{
    if (!m_pendingDragActivation.isValid()) {
        return;
    }

    auto *placesModel = static_cast<KFilePlacesModel *>(model());
    emit placeActivated(KFilePlacesModel::convertedUrl(placesModel->url(m_pendingDragActivation)));
}

void PlacesPanel::slotContextMenuAboutToShow(const QModelIndex &index, QMenu *menu)
{
    if (!m_customContextMenuActions.isEmpty()) {
        menu->addSeparator();
        menu->addActions(m_customContextMenuActions);
    }
}

void PlacesPanel::slotTeardownRequested(const QModelIndex &index)
{
    auto *placesModel = static_cast<KFilePlacesModel *>(model());
    Solid::Device device = placesModel->deviceForIndex(index);

    Solid::StorageAccess *storageAccess = device.as<Solid::StorageAccess>();
    Q_ASSERT(storageAccess);

    // disconnect the Solid::StorageAccess::teardownRequested
    // to prevent emitting PlacesPanel::storageTearDownExternallyRequested
    // after we have emitted PlacesPanel::storageTearDownRequested
    disconnect(storageAccess, &Solid::StorageAccess::teardownRequested, this, nullptr);

    m_indexToTeardown = index;
    storageTearDownRequested(storageAccess->filePath());
}

void PlacesPanel::slotRowsInserted(const QModelIndex &parent, int first, int last)
{
    for (int i = first; i <= last; ++i) {
        connectDeviceSignals(model()->index(first, 0, parent));
    }
}

void PlacesPanel::slotRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    auto *placesModel = static_cast<KFilePlacesModel *>(model());

    for (int i = first; i <= last; ++i) {
        const QModelIndex index = placesModel->index(i, 0, parent);

        Solid::StorageAccess *storageAccess = placesModel->deviceForIndex(index).as<Solid::StorageAccess>();
        if (!storageAccess) {
            continue;
        }

        disconnect(storageAccess, &Solid::StorageAccess::teardownRequested, this, nullptr);
    }
}

void PlacesPanel::connectDeviceSignals(const QModelIndex &index)
{
    auto *placesModel = static_cast<KFilePlacesModel *>(model());

    Solid::StorageAccess *storageAccess = placesModel->deviceForIndex(index).as<Solid::StorageAccess>();
    if (!storageAccess) {
        return;
    }

    connect(storageAccess, &Solid::StorageAccess::teardownRequested, this, [this, storageAccess] {
        emit storageTearDownExternallyRequested(storageAccess->filePath());
    });
}

#include "placespanel.moc"
