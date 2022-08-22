/*
 * SPDX-FileCopyrightText: 2008-2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>
 *
 * Based on KFilePlacesView from kdelibs:
 * SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>
 * SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placespanel.h"

#include "dolphinplacesmodelsingleton.h"
#include "dolphin_generalsettings.h"
#include "dolphin_placespanelsettings.h"
#include "global.h"
#include "views/draganddrophelper.h"
#include "settings/dolphinsettingsdialog.h"

#include <KFilePlacesModel>
#include <KIO/DropJob>
#include <KIO/Job>
#include <KListOpenFilesJob>
#include <KLocalizedString>
#include <KProtocolManager>

#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QShowEvent>
#include <QTimer>

#include <Solid/StorageAccess>

PlacesPanel::PlacesPanel(QWidget* parent)
    : KFilePlacesView(parent)
{
    setDropOnPlaceEnabled(true);
    connect(this, &PlacesPanel::urlsDropped,
            this, &PlacesPanel::slotUrlsDropped);

    setAutoResizeItemsEnabled(false);

    setTeardownFunction([this](const QModelIndex &index) {
        slotTearDownRequested(index);
    });

    m_configureTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18nc("@action:inmenu", "Configure Trash…"));
    m_configureTrashAction->setPriority(QAction::HighPriority);
    connect(m_configureTrashAction, &QAction::triggered, this, &PlacesPanel::slotConfigureTrash);
    addAction(m_configureTrashAction);

    connect(this, &PlacesPanel::contextMenuAboutToShow, this, &PlacesPanel::slotContextMenuAboutToShow);

    connect(this, &PlacesPanel::iconSizeChanged, this, [this](const QSize &newSize) {
        int iconSize = qMin(newSize.width(), newSize.height());
        if (iconSize == 0) {
            // Don't store 0 size, let's keep -1 for default/small/automatic
            iconSize = -1;
        }
        PlacesPanelSettings* settings = PlacesPanelSettings::self();
        settings->setIconSize(iconSize);
        settings->save();
    });
}

PlacesPanel::~PlacesPanel() = default;

void PlacesPanel::setUrl(const QUrl &url)
{
    // KFilePlacesView::setUrl no-ops when no model is set but we only set it in showEvent()
    // Remember the URL and set it in showEvent
    m_url = url;
    KFilePlacesView::setUrl(url);
}

QList<QAction*> PlacesPanel::customContextMenuActions() const
{
    return m_customContextMenuActions;
}

void PlacesPanel::setCustomContextMenuActions(const QList<QAction *> &actions)
{
    m_customContextMenuActions = actions;
}

void PlacesPanel::proceedWithTearDown()
{
    Q_ASSERT(m_deviceToTearDown);

    connect(m_deviceToTearDown, &Solid::StorageAccess::teardownDone,
            this, &PlacesPanel::slotTearDownDone);
    m_deviceToTearDown->teardown();
}

void PlacesPanel::readSettings()
{
    if (GeneralSettings::autoExpandFolders()) {
        setDragAutoActivationDelay(750);
    } else {
        setDragAutoActivationDelay(0);
    }

    const int iconSize = qMax(0, PlacesPanelSettings::iconSize());
    setIconSize(QSize(iconSize, iconSize));
}

void PlacesPanel::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !model()) {
        readSettings();

        auto *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
        setModel(placesModel);

        connect(placesModel, &KFilePlacesModel::errorMessage, this, &PlacesPanel::errorMessage);

        connect(placesModel, &QAbstractItemModel::rowsInserted, this, &PlacesPanel::slotRowsInserted);
        connect(placesModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &PlacesPanel::slotRowsAboutToBeRemoved);

        for (int i = 0; i < model()->rowCount(); ++i) {
            connectDeviceSignals(model()->index(i, 0, QModelIndex()));
        }

        setUrl(m_url);
    }

    KFilePlacesView::showEvent(event);
}

static bool isInternalDrag(const QMimeData *mimeData)
{
    const auto formats = mimeData->formats();
    for (const auto &format : formats) {
        // from KFilePlacesModel::_k_internalMimetype
        if (format.startsWith(QLatin1String("application/x-kfileplacesmodel-"))) {
            return true;
        }
    }
    return false;
}

void PlacesPanel::dragMoveEvent(QDragMoveEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        auto *placesModel = static_cast<KFilePlacesModel *>(model());

        // Reject drag ontop of a non-writable protocol
        // We don't know whether we're dropping inbetween or ontop of a place
        // so still allow internal drag events so that re-arranging still works.
        const QUrl url = placesModel->url(index);
        if (url.isValid() && !isInternalDrag(event->mimeData()) && !KProtocolManager::supportsWriting(url)) {
            event->setDropAction(Qt::IgnoreAction);
        }
    }

    KFilePlacesView::dragMoveEvent(event);
}

void PlacesPanel::slotConfigureTrash()
{
    const QUrl url = currentIndex().data(KFilePlacesModel::UrlRole).toUrl();

    DolphinSettingsDialog* settingsDialog = new DolphinSettingsDialog(url, this);
    settingsDialog->setCurrentPage(settingsDialog->trashSettings);
    settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
    settingsDialog->show();
}

void PlacesPanel::slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent)
{
    KIO::DropJob *job = DragAndDropHelper::dropUrls(dest, event, parent);
    if (job) {
        connect(job, &KIO::DropJob::result, this, [this](KJob *job) { if (job->error()) Q_EMIT errorMessage(job->errorString()); });
    }
}

void PlacesPanel::slotContextMenuAboutToShow(const QModelIndex &index, QMenu *menu)
{
    Q_UNUSED(menu);

    auto *placesModel = static_cast<KFilePlacesModel *>(model());
    const QUrl url = placesModel->url(index);
    const Solid::Device device = placesModel->deviceForIndex(index);

    m_configureTrashAction->setVisible(url.scheme() == QLatin1String("trash"));

    // show customContextMenuActions only on the view's context menu
    if (!url.isValid() && !device.isValid()) {
        addActions(m_customContextMenuActions);
    } else {
        const auto actions = this->actions();
        for (QAction *action : actions) {
            if (m_customContextMenuActions.contains(action)) {
                removeAction(action);
            }
        }
    }
}

void PlacesPanel::slotTearDownRequested(const QModelIndex &index)
{
    auto *placesModel = static_cast<KFilePlacesModel *>(model());

    Solid::StorageAccess *storageAccess = placesModel->deviceForIndex(index).as<Solid::StorageAccess>();
    if (!storageAccess) {
        return;
    }

    m_deviceToTearDown = storageAccess;

    // disconnect the Solid::StorageAccess::teardownRequested
    // to prevent emitting PlacesPanel::storageTearDownExternallyRequested
    // after we have emitted PlacesPanel::storageTearDownRequested
    disconnect(storageAccess, &Solid::StorageAccess::teardownRequested, this, &PlacesPanel::slotTearDownRequestedExternally);
    Q_EMIT storageTearDownRequested(storageAccess->filePath());
}

void PlacesPanel::slotTearDownRequestedExternally(const QString &udi)
{
    Q_UNUSED(udi);
    auto *storageAccess = static_cast<Solid::StorageAccess*>(sender());

    Q_EMIT storageTearDownExternallyRequested(storageAccess->filePath());
}

void PlacesPanel::slotTearDownDone(Solid::ErrorType error, const QVariant& errorData)
{
    if (error && errorData.isValid()) {
        if (error == Solid::ErrorType::UserCanceled) {
            // No need to tell the user what they just did.
        } else if (error == Solid::ErrorType::DeviceBusy) {
            KListOpenFilesJob* listOpenFilesJob = new KListOpenFilesJob(m_deviceToTearDown->filePath());
            connect(listOpenFilesJob, &KIO::Job::result, this, [this, listOpenFilesJob](KJob*) {
                const KProcessList::KProcessInfoList blockingProcesses = listOpenFilesJob->processInfoList();
                QString errorString;
                if (blockingProcesses.isEmpty()) {
                    errorString = i18n("One or more files on this device are open within an application.");
                } else {
                    QStringList blockingApps;
                    for (const auto& process : blockingProcesses) {
                        blockingApps << process.name();
                    }
                    blockingApps.removeDuplicates();
                    errorString = xi18np("One or more files on this device are opened in application <application>\"%2\"</application>.",
                            "One or more files on this device are opened in following applications: <application>%2</application>.",
                            blockingApps.count(), blockingApps.join(i18nc("separator in list of apps blocking device unmount", ", ")));
                }
                Q_EMIT errorMessage(errorString);
            });
            listOpenFilesJob->start();
        } else {
            Q_EMIT errorMessage(errorData.toString());
        }
    } else {
        // No error; it must have been unmounted successfully
        Q_EMIT storageTearDownSuccessful();
    }
    disconnect(m_deviceToTearDown, &Solid::StorageAccess::teardownDone,
               this, &PlacesPanel::slotTearDownDone);
    m_deviceToTearDown = nullptr;
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

    connect(storageAccess, &Solid::StorageAccess::teardownRequested, this, &PlacesPanel::slotTearDownRequestedExternally);
}
