/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemmodelrolesupdater.h"

#include "dolphindebug.h"
#include "kfileitemmodel.h"
#include "private/kdirectorycontentscounter.h"
#include "private/kpixmapmodifier.h"

#include <KConfig>
#include <KConfigGroup>
#include <KIO/PreviewJob>
#include <KIconLoader>
#include <KJobWidgets>
#include <KOverlayIconPlugin>
#include <KPluginMetaData>
#include <KSharedConfig>

#if HAVE_BALOO
#include "private/kbaloorolesprovider.h"
#include <Baloo/File>
#include <Baloo/FileMonitor>
#endif

#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QPainter>
#include <QPluginLoader>
#include <QTimer>
#include <chrono>

using namespace std::chrono_literals;

// #define KFILEITEMMODELROLESUPDATER_DEBUG

namespace
{
// Maximum time in ms that the KFileItemModelRolesUpdater
// may perform a blocking operation
const int MaxBlockTimeout = 200;

// If the number of items is smaller than ResolveAllItemsLimit,
// the roles of all items will be resolved.
const int ResolveAllItemsLimit = 500;

// Not only the visible area, but up to ReadAheadPages before and after
// this area will be resolved.
const int ReadAheadPages = 5;
}

KFileItemModelRolesUpdater::KFileItemModelRolesUpdater(KFileItemModel *model, QObject *parent)
    : QObject(parent)
    , m_state(Idle)
    , m_previewChangedDuringPausing(false)
    , m_iconSizeChangedDuringPausing(false)
    , m_rolesChangedDuringPausing(false)
    , m_previewShown(false)
    , m_enlargeSmallPreviews(true)
    , m_clearPreviews(false)
    , m_finishedItems()
    , m_model(model)
    , m_iconSize()
    , m_firstVisibleIndex(0)
    , m_lastVisibleIndex(-1)
    , m_maximumVisibleItems(50)
    , m_roles()
    , m_resolvableRoles()
    , m_enabledPlugins()
    , m_localFileSizePreviewLimit(0)
    , m_scanDirectories(true)
    , m_pendingSortRoleItems()
    , m_pendingIndexes()
    , m_pendingPreviewItems()
    , m_previewJob()
    , m_hoverSequenceItem()
    , m_hoverSequenceIndex(0)
    , m_hoverSequencePreviewJob(nullptr)
    , m_hoverSequenceNumSuccessiveFailures(0)
    , m_recentlyChangedItemsTimer(nullptr)
    , m_recentlyChangedItems()
    , m_changedItems()
    , m_directoryContentsCounter(nullptr)
#if HAVE_BALOO
    , m_balooFileMonitor(nullptr)
#endif
{
    Q_ASSERT(model);

    const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
    m_enabledPlugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());
    m_localFileSizePreviewLimit = static_cast<qulonglong>(globalConfig.readEntry("MaximumSize", 0));

    connect(m_model, &KFileItemModel::itemsInserted, this, &KFileItemModelRolesUpdater::slotItemsInserted);
    connect(m_model, &KFileItemModel::itemsRemoved, this, &KFileItemModelRolesUpdater::slotItemsRemoved);
    connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
    connect(m_model, &KFileItemModel::itemsMoved, this, &KFileItemModelRolesUpdater::slotItemsMoved);
    connect(m_model, &KFileItemModel::sortRoleChanged, this, &KFileItemModelRolesUpdater::slotSortRoleChanged);

    // Use a timer to prevent that each call of slotItemsChanged() results in a synchronous
    // resolving of the roles. Postpone the resolving until no update has been done for 100 ms.
    m_recentlyChangedItemsTimer = new QTimer(this);
    m_recentlyChangedItemsTimer->setInterval(100ms);
    m_recentlyChangedItemsTimer->setSingleShot(true);
    connect(m_recentlyChangedItemsTimer, &QTimer::timeout, this, &KFileItemModelRolesUpdater::resolveRecentlyChangedItems);

    m_resolvableRoles.insert("size");
    m_resolvableRoles.insert("type");
    m_resolvableRoles.insert("isExpandable");
#if HAVE_BALOO
    m_resolvableRoles += KBalooRolesProvider::instance().roles();
#endif

    m_directoryContentsCounter = new KDirectoryContentsCounter(m_model, this);
    connect(m_directoryContentsCounter, &KDirectoryContentsCounter::result, this, &KFileItemModelRolesUpdater::slotDirectoryContentsCountReceived);

    const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR)) + QStringLiteral("/overlayicon"));
    for (const KPluginMetaData &data : plugins) {
        auto instance = QPluginLoader(data.fileName()).instance();
        auto plugin = qobject_cast<KOverlayIconPlugin *>(instance);
        if (plugin) {
            m_overlayIconsPlugin.append(plugin);
            connect(plugin, &KOverlayIconPlugin::overlaysChanged, this, &KFileItemModelRolesUpdater::slotOverlaysChanged);
        } else {
            // not our/valid plugin, so delete the created object
            delete instance;
        }
    }
}

KFileItemModelRolesUpdater::~KFileItemModelRolesUpdater()
{
    killPreviewJob();
}

void KFileItemModelRolesUpdater::setIconSize(const QSize &size)
{
    if (size != m_iconSize) {
        m_iconSize = size;
        if (m_state == Paused) {
            m_iconSizeChangedDuringPausing = true;
        } else if (m_previewShown) {
            // An icon size change requires the regenerating of
            // all previews
            m_finishedItems.clear();
            startUpdating();
        }
    }
}

QSize KFileItemModelRolesUpdater::iconSize() const
{
    return m_iconSize;
}

void KFileItemModelRolesUpdater::setVisibleIndexRange(int index, int count)
{
    if (index < 0) {
        index = 0;
    }
    if (count < 0) {
        count = 0;
    }

    if (index == m_firstVisibleIndex && count == m_lastVisibleIndex - m_firstVisibleIndex + 1) {
        // The range has not been changed
        return;
    }

    m_firstVisibleIndex = index;
    m_lastVisibleIndex = qMin(index + count - 1, m_model->count() - 1);

    startUpdating();
}

void KFileItemModelRolesUpdater::setMaximumVisibleItems(int count)
{
    m_maximumVisibleItems = count;
}

void KFileItemModelRolesUpdater::setPreviewsShown(bool show)
{
    if (show == m_previewShown) {
        return;
    }

    m_previewShown = show;
    if (!show) {
        m_clearPreviews = true;
    }

    updateAllPreviews();
}

bool KFileItemModelRolesUpdater::previewsShown() const
{
    return m_previewShown;
}

void KFileItemModelRolesUpdater::setEnlargeSmallPreviews(bool enlarge)
{
    if (enlarge != m_enlargeSmallPreviews) {
        m_enlargeSmallPreviews = enlarge;
        if (m_previewShown) {
            updateAllPreviews();
        }
    }
}

bool KFileItemModelRolesUpdater::enlargeSmallPreviews() const
{
    return m_enlargeSmallPreviews;
}

void KFileItemModelRolesUpdater::setEnabledPlugins(const QStringList &list)
{
    if (m_enabledPlugins != list) {
        m_enabledPlugins = list;
        if (m_previewShown) {
            updateAllPreviews();
        }
    }
}

void KFileItemModelRolesUpdater::setPaused(bool paused)
{
    if (paused == (m_state == Paused)) {
        return;
    }

    if (paused) {
        m_state = Paused;
        killPreviewJob();
    } else {
        const bool updatePreviews = (m_iconSizeChangedDuringPausing && m_previewShown) || m_previewChangedDuringPausing;
        const bool resolveAll = updatePreviews || m_rolesChangedDuringPausing;
        if (resolveAll) {
            m_finishedItems.clear();
        }

        m_iconSizeChangedDuringPausing = false;
        m_previewChangedDuringPausing = false;
        m_rolesChangedDuringPausing = false;

        if (!m_pendingSortRoleItems.isEmpty()) {
            m_state = ResolvingSortRole;
            resolveNextSortRole();
        } else {
            m_state = Idle;
        }

        startUpdating();
    }
}

void KFileItemModelRolesUpdater::setRoles(const QSet<QByteArray> &roles)
{
    if (m_roles != roles) {
        m_roles = roles;

#if HAVE_BALOO
        // Check whether there is at least one role that must be resolved
        // with the help of Baloo. If this is the case, a (quite expensive)
        // resolving will be done in KFileItemModelRolesUpdater::rolesData() and
        // the role gets watched for changes.
        const KBalooRolesProvider &rolesProvider = KBalooRolesProvider::instance();
        bool hasBalooRole = false;
        QSetIterator<QByteArray> it(roles);
        while (it.hasNext()) {
            const QByteArray &role = it.next();
            if (rolesProvider.roles().contains(role)) {
                hasBalooRole = true;
                break;
            }
        }

        if (hasBalooRole && m_balooConfig.fileIndexingEnabled() && !m_balooFileMonitor) {
            m_balooFileMonitor = new Baloo::FileMonitor(this);
            connect(m_balooFileMonitor, &Baloo::FileMonitor::fileMetaDataChanged, this, &KFileItemModelRolesUpdater::applyChangedBalooRoles);
        } else if (!hasBalooRole && m_balooFileMonitor) {
            delete m_balooFileMonitor;
            m_balooFileMonitor = nullptr;
        }
#endif

        if (m_state == Paused) {
            m_rolesChangedDuringPausing = true;
        } else {
            startUpdating();
        }
    }
}

QSet<QByteArray> KFileItemModelRolesUpdater::roles() const
{
    return m_roles;
}

bool KFileItemModelRolesUpdater::isPaused() const
{
    return m_state == Paused;
}

QStringList KFileItemModelRolesUpdater::enabledPlugins() const
{
    return m_enabledPlugins;
}

void KFileItemModelRolesUpdater::setLocalFileSizePreviewLimit(const qlonglong size)
{
    m_localFileSizePreviewLimit = size;
}

qlonglong KFileItemModelRolesUpdater::localFileSizePreviewLimit() const
{
    return m_localFileSizePreviewLimit;
}

void KFileItemModelRolesUpdater::setScanDirectories(bool enabled)
{
    m_scanDirectories = enabled;
}

bool KFileItemModelRolesUpdater::scanDirectories() const
{
    return m_scanDirectories;
}

void KFileItemModelRolesUpdater::setHoverSequenceState(const QUrl &itemUrl, int seqIdx)
{
    const KFileItem item = m_model->fileItem(itemUrl);

    if (item != m_hoverSequenceItem) {
        killHoverSequencePreviewJob();
    }

    m_hoverSequenceItem = item;
    m_hoverSequenceIndex = seqIdx;

    if (!m_previewShown) {
        return;
    }

    m_hoverSequenceNumSuccessiveFailures = 0;

    loadNextHoverSequencePreview();
}

void KFileItemModelRolesUpdater::slotItemsInserted(const KItemRangeList &itemRanges)
{
    QElapsedTimer timer;
    timer.start();

    // Determine the sort role synchronously for as many items as possible.
    if (m_resolvableRoles.contains(m_model->sortRole())) {
        int insertedCount = 0;
        for (const KItemRange &range : itemRanges) {
            const int lastIndex = insertedCount + range.index + range.count - 1;
            for (int i = insertedCount + range.index; i <= lastIndex; ++i) {
                if (timer.elapsed() < MaxBlockTimeout) {
                    applySortRole(i);
                } else {
                    m_pendingSortRoleItems.insert(m_model->fileItem(i));
                }
            }
            insertedCount += range.count;
        }

        applySortProgressToModel();

        // If there are still items whose sort role is unknown, check if the
        // asynchronous determination of the sort role is already in progress,
        // and start it if that is not the case.
        if (!m_pendingSortRoleItems.isEmpty() && m_state != ResolvingSortRole) {
            killPreviewJob();
            m_state = ResolvingSortRole;
            resolveNextSortRole();
        }
    }

    startUpdating();
}

void KFileItemModelRolesUpdater::slotItemsRemoved(const KItemRangeList &itemRanges)
{
    Q_UNUSED(itemRanges)

    const bool allItemsRemoved = (m_model->count() == 0);

#if HAVE_BALOO
    if (m_balooFileMonitor) {
        // Don't let the FileWatcher watch for removed items
        if (allItemsRemoved) {
            m_balooFileMonitor->clear();
        } else {
            QStringList newFileList;
            const QStringList oldFileList = m_balooFileMonitor->files();
            for (const QString &file : oldFileList) {
                if (m_model->index(QUrl::fromLocalFile(file)) >= 0) {
                    newFileList.append(file);
                }
            }
            m_balooFileMonitor->setFiles(newFileList);
        }
    }
#endif

    if (allItemsRemoved) {
        m_state = Idle;

        m_finishedItems.clear();
        m_pendingSortRoleItems.clear();
        m_pendingIndexes.clear();
        m_pendingPreviewItems.clear();
        m_recentlyChangedItems.clear();
        m_recentlyChangedItemsTimer->stop();
        m_changedItems.clear();
        m_hoverSequenceLoadedItems.clear();

        killPreviewJob();
    } else {
        // Only remove the items from m_finishedItems. They will be removed
        // from the other sets later on.
        QSet<KFileItem>::iterator it = m_finishedItems.begin();
        while (it != m_finishedItems.end()) {
            if (m_model->index(*it) < 0) {
                it = m_finishedItems.erase(it);
            } else {
                ++it;
            }
        }

        // Removed items won't have hover previews loaded anymore.
        for (const KItemRange &itemRange : itemRanges) {
            int index = itemRange.index;
            for (int count = itemRange.count; count > 0; --count) {
                const KFileItem item = m_model->fileItem(index);
                m_hoverSequenceLoadedItems.remove(item);
                ++index;
            }
        }

        // The visible items might have changed.
        startUpdating();
    }
}

void KFileItemModelRolesUpdater::slotItemsMoved(KItemRange itemRange, const QList<int> &movedToIndexes)
{
    Q_UNUSED(itemRange)
    Q_UNUSED(movedToIndexes)

    // The visible items might have changed.
    startUpdating();
}

void KFileItemModelRolesUpdater::slotItemsChanged(const KItemRangeList &itemRanges, const QSet<QByteArray> &roles)
{
    Q_UNUSED(roles)

    // Find out if slotItemsChanged() has been done recently. If that is the
    // case, resolving the roles is postponed until a timer has exceeded
    // to prevent expensive repeated updates if files are updated frequently.
    const bool itemsChangedRecently = m_recentlyChangedItemsTimer->isActive();

    QSet<KFileItem> &targetSet = itemsChangedRecently ? m_recentlyChangedItems : m_changedItems;

    for (const KItemRange &itemRange : itemRanges) {
        int index = itemRange.index;
        for (int count = itemRange.count; count > 0; --count) {
            const KFileItem item = m_model->fileItem(index);
            targetSet.insert(item);
            ++index;
        }
    }

    m_recentlyChangedItemsTimer->start();

    if (!itemsChangedRecently) {
        updateChangedItems();
    }
}

void KFileItemModelRolesUpdater::slotSortRoleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    if (m_resolvableRoles.contains(current)) {
        m_pendingSortRoleItems.clear();
        m_finishedItems.clear();

        const int count = m_model->count();
        QElapsedTimer timer;
        timer.start();

        // Determine the sort role synchronously for as many items as possible.
        for (int index = 0; index < count; ++index) {
            if (timer.elapsed() < MaxBlockTimeout) {
                applySortRole(index);
            } else {
                m_pendingSortRoleItems.insert(m_model->fileItem(index));
            }
        }

        applySortProgressToModel();

        if (!m_pendingSortRoleItems.isEmpty()) {
            // Trigger the asynchronous determination of the sort role.
            killPreviewJob();
            m_state = ResolvingSortRole;
            resolveNextSortRole();
        }
    } else {
        m_state = Idle;
        m_pendingSortRoleItems.clear();
        applySortProgressToModel();
    }
}

void KFileItemModelRolesUpdater::slotGotPreview(const KFileItem &item, const QPixmap &pixmap)
{
    if (m_state != PreviewJobRunning) {
        return;
    }

    m_changedItems.remove(item);

    const int index = m_model->index(item);
    if (index < 0) {
        return;
    }

    QPixmap scaledPixmap = transformPreviewPixmap(pixmap);

    QHash<QByteArray, QVariant> data = rolesData(item);

    const QStringList overlays = data["iconOverlays"].toStringList();
    // Strangely KFileItem::overlays() returns empty string-values, so
    // we need to check first whether an overlay must be drawn at all.
    // It is more efficient to do it here, as KIconLoader::drawOverlays()
    // assumes that an overlay will be drawn and has some additional
    // setup time.
    if (!scaledPixmap.isNull()) {
        for (const QString &overlay : overlays) {
            if (!overlay.isEmpty()) {
                // There is at least one overlay, draw all overlays above m_pixmap
                // and cancel the check
                KIconLoader::global()->drawOverlays(overlays, scaledPixmap, KIconLoader::Desktop);
                break;
            }
        }
    }

    data.insert("iconPixmap", scaledPixmap);

    disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
    m_model->setData(index, data);
    connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);

    m_finishedItems.insert(item);
}

void KFileItemModelRolesUpdater::slotPreviewFailed(const KFileItem &item)
{
    if (m_state != PreviewJobRunning) {
        return;
    }

    m_changedItems.remove(item);

    const int index = m_model->index(item);
    if (index >= 0) {
        QHash<QByteArray, QVariant> data;
        data.insert("iconPixmap", QPixmap());

        disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
        m_model->setData(index, data);
        connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);

        applyResolvedRoles(index, ResolveAll);
        m_finishedItems.insert(item);
    }
}

void KFileItemModelRolesUpdater::slotPreviewJobFinished()
{
    m_previewJob = nullptr;

    if (m_state != PreviewJobRunning) {
        return;
    }

    m_state = Idle;

    if (!m_pendingPreviewItems.isEmpty()) {
        startPreviewJob();
    } else {
        if (!m_changedItems.isEmpty()) {
            updateChangedItems();
        }
    }
}

void KFileItemModelRolesUpdater::slotHoverSequenceGotPreview(const KFileItem &item, const QPixmap &pixmap)
{
    const int index = m_model->index(item);
    if (index < 0) {
        return;
    }

    QHash<QByteArray, QVariant> data = m_model->data(index);
    QVector<QPixmap> pixmaps = data["hoverSequencePixmaps"].value<QVector<QPixmap>>();
    const int loadedIndex = pixmaps.size();

    float wap = m_hoverSequencePreviewJob->sequenceIndexWraparoundPoint();
    if (!m_hoverSequencePreviewJob->handlesSequences()) {
        wap = 1.0f;
    }
    if (wap >= 0.0f) {
        data["hoverSequenceWraparoundPoint"] = wap;
        m_model->setData(index, data);
    }

    // For hover sequence previews we never load index 0, because that's just the regular preview
    // in "iconPixmap". But that means we'll load index 1 even for thumbnailers that don't support
    // sequences, in which case we can just throw away the preview because it's the same as for
    // index 0. Unfortunately we can't find it out earlier :(
    if (wap < 0.0f || loadedIndex < static_cast<int>(wap)) {
        // Add the preview to the model data

        const QPixmap scaledPixmap = transformPreviewPixmap(pixmap);

        pixmaps.append(scaledPixmap);
        data["hoverSequencePixmaps"] = QVariant::fromValue(pixmaps);

        m_model->setData(index, data);

        const auto loadedIt = std::find(m_hoverSequenceLoadedItems.begin(), m_hoverSequenceLoadedItems.end(), item);
        if (loadedIt == m_hoverSequenceLoadedItems.end()) {
            m_hoverSequenceLoadedItems.push_back(item);
            trimHoverSequenceLoadedItems();
        }
    }

    m_hoverSequenceNumSuccessiveFailures = 0;
}

void KFileItemModelRolesUpdater::slotHoverSequencePreviewFailed(const KFileItem &item)
{
    const int index = m_model->index(item);
    if (index < 0) {
        return;
    }

    static const int numRetries = 2;

    QHash<QByteArray, QVariant> data = m_model->data(index);
    QVector<QPixmap> pixmaps = data["hoverSequencePixmaps"].value<QVector<QPixmap>>();

    qCDebug(DolphinDebug).nospace() << "Failed to generate hover sequence preview #" << pixmaps.size() << " for file " << item.url().toString() << " (attempt "
                                    << (m_hoverSequenceNumSuccessiveFailures + 1) << "/" << (numRetries + 1) << ")";

    if (m_hoverSequenceNumSuccessiveFailures >= numRetries) {
        // Give up and simply duplicate the previous sequence image (if any)

        pixmaps.append(pixmaps.empty() ? QPixmap() : pixmaps.last());
        data["hoverSequencePixmaps"] = QVariant::fromValue(pixmaps);

        if (!data.contains("hoverSequenceWraparoundPoint")) {
            // hoverSequenceWraparoundPoint is only available when PreviewJob succeeds, so unless
            // it has previously succeeded, it's best to assume that it just doesn't handle
            // sequences instead of trying to load the next image indefinitely.
            data["hoverSequenceWraparoundPoint"] = 1.0f;
        }

        m_model->setData(index, data);

        m_hoverSequenceNumSuccessiveFailures = 0;
    } else {
        // Retry

        m_hoverSequenceNumSuccessiveFailures++;
    }

    // Next image in the sequence (or same one if the retry limit wasn't reached yet) will be
    // loaded automatically, because slotHoverSequencePreviewJobFinished() will be triggered
    // even when PreviewJob fails.
}

void KFileItemModelRolesUpdater::slotHoverSequencePreviewJobFinished()
{
    const int index = m_model->index(m_hoverSequenceItem);
    if (index < 0) {
        m_hoverSequencePreviewJob = nullptr;
        return;
    }

    // Since a PreviewJob can only have one associated sequence index, we can only generate
    // one sequence image per job, so we have to start another one for the next index.

    // Load the next image in the sequence
    m_hoverSequencePreviewJob = nullptr;
    loadNextHoverSequencePreview();
}

void KFileItemModelRolesUpdater::resolveNextSortRole()
{
    if (m_state != ResolvingSortRole) {
        return;
    }

    QSet<KFileItem>::iterator it = m_pendingSortRoleItems.begin();
    while (it != m_pendingSortRoleItems.end()) {
        const KFileItem item = *it;
        const int index = m_model->index(item);

        // Continue if the sort role has already been determined for the
        // item, and the item has not been changed recently.
        if (!m_changedItems.contains(item) && m_model->data(index).contains(m_model->sortRole())) {
            it = m_pendingSortRoleItems.erase(it);
            continue;
        }

        applySortRole(index);
        m_pendingSortRoleItems.erase(it);
        break;
    }

    if (!m_pendingSortRoleItems.isEmpty()) {
        applySortProgressToModel();
        QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::resolveNextSortRole);
    } else {
        m_state = Idle;

        // Prevent that we try to update the items twice.
        disconnect(m_model, &KFileItemModel::itemsMoved, this, &KFileItemModelRolesUpdater::slotItemsMoved);
        applySortProgressToModel();
        connect(m_model, &KFileItemModel::itemsMoved, this, &KFileItemModelRolesUpdater::slotItemsMoved);
        startUpdating();
    }
}

void KFileItemModelRolesUpdater::resolveNextPendingRoles()
{
    if (m_state != ResolvingAllRoles) {
        return;
    }

    while (!m_pendingIndexes.isEmpty()) {
        const int index = m_pendingIndexes.takeFirst();
        const KFileItem item = m_model->fileItem(index);

        if (m_finishedItems.contains(item)) {
            continue;
        }

        applyResolvedRoles(index, ResolveAll);
        m_finishedItems.insert(item);
        m_changedItems.remove(item);
        break;
    }

    if (!m_pendingIndexes.isEmpty()) {
        QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::resolveNextPendingRoles);
    } else {
        m_state = Idle;

        if (m_clearPreviews) {
            // Only go through the list if there are items which might still have previews.
            if (m_finishedItems.count() != m_model->count()) {
                QHash<QByteArray, QVariant> data;
                data.insert("iconPixmap", QPixmap());
                data.insert("hoverSequencePixmaps", QVariant::fromValue(QVector<QPixmap>()));

                disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
                for (int index = 0; index <= m_model->count(); ++index) {
                    if (m_model->data(index).contains("iconPixmap") || m_model->data(index).contains("hoverSequencePixmaps")) {
                        m_model->setData(index, data);
                    }
                }
                connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
            }
            m_clearPreviews = false;
        }

        if (!m_changedItems.isEmpty()) {
            updateChangedItems();
        }
    }
}

void KFileItemModelRolesUpdater::resolveRecentlyChangedItems()
{
    m_changedItems += m_recentlyChangedItems;
    m_recentlyChangedItems.clear();
    updateChangedItems();
}

void KFileItemModelRolesUpdater::applyChangedBalooRoles(const QString &file)
{
#if HAVE_BALOO
    const KFileItem item = m_model->fileItem(QUrl::fromLocalFile(file));

    if (item.isNull()) {
        // itemUrl is not in the model anymore, probably because
        // the corresponding file has been deleted in the meantime.
        return;
    }
    applyChangedBalooRolesForItem(item);
#else
    Q_UNUSED(file)
#endif
}

void KFileItemModelRolesUpdater::applyChangedBalooRolesForItem(const KFileItem &item)
{
#if HAVE_BALOO
    Baloo::File file(item.localPath());
    file.load();

    const KBalooRolesProvider &rolesProvider = KBalooRolesProvider::instance();
    QHash<QByteArray, QVariant> data;

    const auto roles = rolesProvider.roles();
    for (const QByteArray &role : roles) {
        // Overwrite all the role values with an empty QVariant, because the roles
        // provider doesn't overwrite it when the property value list is empty.
        // See bug 322348
        data.insert(role, QVariant());
    }

    QHashIterator<QByteArray, QVariant> it(rolesProvider.roleValues(file, m_roles));
    while (it.hasNext()) {
        it.next();
        data.insert(it.key(), it.value());
    }

    disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
    const int index = m_model->index(item);
    m_model->setData(index, data);
    connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
#else
#ifndef Q_CC_MSVC
    Q_UNUSED(item)
#endif
#endif
}

void KFileItemModelRolesUpdater::slotDirectoryContentsCountReceived(const QString &path, int count, long size)
{
    const bool getSizeRole = m_roles.contains("size");
    const bool getIsExpandableRole = m_roles.contains("isExpandable");

    if (getSizeRole || getIsExpandableRole) {
        const int index = m_model->index(QUrl::fromLocalFile(path));
        if (index >= 0) {
            QHash<QByteArray, QVariant> data;

            if (getSizeRole) {
                data.insert("count", count);
                data.insert("size", QVariant::fromValue(size));
            }
            if (getIsExpandableRole) {
                data.insert("isExpandable", count > 0);
            }

            disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
            m_model->setData(index, data);
            connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
        }
    }
}

void KFileItemModelRolesUpdater::startUpdating()
{
    if (m_state == Paused) {
        return;
    }

    if (m_finishedItems.count() == m_model->count()) {
        // All roles have been resolved already.
        m_state = Idle;
        return;
    }

    // Terminate all updates that are currently active.
    killPreviewJob();
    m_pendingIndexes.clear();

    QElapsedTimer timer;
    timer.start();

    // Determine the icons for the visible items synchronously.
    updateVisibleIcons();

    // A detailed update of the items in and near the visible area
    // only makes sense if sorting is finished.
    if (m_state == ResolvingSortRole) {
        return;
    }

    // Start the preview job or the asynchronous resolving of all roles.
    QList<int> indexes = indexesToResolve();

    if (m_previewShown) {
        m_pendingPreviewItems.clear();
        m_pendingPreviewItems.reserve(indexes.count());

        for (int index : qAsConst(indexes)) {
            const KFileItem item = m_model->fileItem(index);
            if (!m_finishedItems.contains(item)) {
                m_pendingPreviewItems.append(item);
            }
        }

        startPreviewJob();
    } else {
        m_pendingIndexes = indexes;
        // Trigger the asynchronous resolving of all roles.
        m_state = ResolvingAllRoles;
        QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::resolveNextPendingRoles);
    }
}

void KFileItemModelRolesUpdater::updateVisibleIcons()
{
    int lastVisibleIndex = m_lastVisibleIndex;
    if (lastVisibleIndex <= 0) {
        // Guess a reasonable value for the last visible index if the view
        // has not told us about the real value yet.
        lastVisibleIndex = qMin(m_firstVisibleIndex + m_maximumVisibleItems, m_model->count() - 1);
        if (lastVisibleIndex <= 0) {
            lastVisibleIndex = qMin(200, m_model->count() - 1);
        }
    }

    QElapsedTimer timer;
    timer.start();

    // Try to determine the final icons for all visible items.
    int index;
    for (index = m_firstVisibleIndex; index <= lastVisibleIndex && timer.elapsed() < MaxBlockTimeout; ++index) {
        applyResolvedRoles(index, ResolveFast);
    }

    // KFileItemListView::initializeItemListWidget(KItemListWidget*) will load
    // preliminary icons (i.e., without mime type determination) for the
    // remaining items.
}

void KFileItemModelRolesUpdater::startPreviewJob()
{
    m_state = PreviewJobRunning;

    if (m_pendingPreviewItems.isEmpty()) {
        QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::slotPreviewJobFinished);
        return;
    }

    // PreviewJob internally caches items always with the size of
    // 128 x 128 pixels or 256 x 256 pixels. A (slow) downscaling is done
    // by PreviewJob if a smaller size is requested. For images KFileItemModelRolesUpdater must
    // do a downscaling anyhow because of the frame, so in this case only the provided
    // cache sizes are requested.
    const QSize cacheSize = (m_iconSize.width() > 128) || (m_iconSize.height() > 128) ? QSize(256, 256) : QSize(128, 128);

    // KIO::filePreview() will request the MIME-type of all passed items, which (in the
    // worst case) might block the application for several seconds. To prevent such
    // a blocking, we only pass items with known mime type to the preview job.
    const int count = m_pendingPreviewItems.count();
    KFileItemList itemSubSet;
    itemSubSet.reserve(count);

    if (m_pendingPreviewItems.first().isMimeTypeKnown()) {
        // Some mime types are known already, probably because they were
        // determined when loading the icons for the visible items. Start
        // a preview job for all items at the beginning of the list which
        // have a known mime type.
        do {
            itemSubSet.append(m_pendingPreviewItems.takeFirst());
        } while (!m_pendingPreviewItems.isEmpty() && m_pendingPreviewItems.first().isMimeTypeKnown());
    } else {
        // Determine mime types for MaxBlockTimeout ms, and start a preview
        // job for the corresponding items.
        QElapsedTimer timer;
        timer.start();

        do {
            const KFileItem item = m_pendingPreviewItems.takeFirst();
            item.determineMimeType();
            itemSubSet.append(item);
        } while (!m_pendingPreviewItems.isEmpty() && timer.elapsed() < MaxBlockTimeout);
    }

    KIO::PreviewJob *job = new KIO::PreviewJob(itemSubSet, cacheSize, &m_enabledPlugins);

    job->setIgnoreMaximumSize(itemSubSet.first().isLocalFile() && !itemSubSet.first().isSlow() && m_localFileSizePreviewLimit <= 0);
    if (job->uiDelegate()) {
        KJobWidgets::setWindow(job, qApp->activeWindow());
    }

    connect(job, &KIO::PreviewJob::gotPreview, this, &KFileItemModelRolesUpdater::slotGotPreview);
    connect(job, &KIO::PreviewJob::failed, this, &KFileItemModelRolesUpdater::slotPreviewFailed);
    connect(job, &KIO::PreviewJob::finished, this, &KFileItemModelRolesUpdater::slotPreviewJobFinished);

    m_previewJob = job;
}

QPixmap KFileItemModelRolesUpdater::transformPreviewPixmap(const QPixmap &pixmap)
{
    QPixmap scaledPixmap = pixmap;

    if (!pixmap.hasAlpha() && !pixmap.isNull() && m_iconSize.width() > KIconLoader::SizeSmallMedium && m_iconSize.height() > KIconLoader::SizeSmallMedium) {
        if (m_enlargeSmallPreviews) {
            KPixmapModifier::applyFrame(scaledPixmap, m_iconSize);
        } else {
            // Assure that small previews don't get enlarged. Instead they
            // should be shown centered within the frame.
            const QSize contentSize = KPixmapModifier::sizeInsideFrame(m_iconSize);
            const bool enlargingRequired = scaledPixmap.width() < contentSize.width() && scaledPixmap.height() < contentSize.height();
            if (enlargingRequired) {
                QSize frameSize = scaledPixmap.size() / scaledPixmap.devicePixelRatio();
                frameSize.scale(m_iconSize, Qt::KeepAspectRatio);

                QPixmap largeFrame(frameSize);
                largeFrame.fill(Qt::transparent);

                KPixmapModifier::applyFrame(largeFrame, frameSize);

                QPainter painter(&largeFrame);
                painter.drawPixmap((largeFrame.width() - scaledPixmap.width() / scaledPixmap.devicePixelRatio()) / 2,
                                   (largeFrame.height() - scaledPixmap.height() / scaledPixmap.devicePixelRatio()) / 2,
                                   scaledPixmap);
                scaledPixmap = largeFrame;
            } else {
                // The image must be shrunk as it is too large to fit into
                // the available icon size
                KPixmapModifier::applyFrame(scaledPixmap, m_iconSize);
            }
        }
    } else if (!pixmap.isNull()) {
        KPixmapModifier::scale(scaledPixmap, m_iconSize * qApp->devicePixelRatio());
        scaledPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    }

    return scaledPixmap;
}

void KFileItemModelRolesUpdater::loadNextHoverSequencePreview()
{
    if (m_hoverSequenceItem.isNull() || m_hoverSequencePreviewJob) {
        return;
    }

    const int index = m_model->index(m_hoverSequenceItem);
    if (index < 0) {
        return;
    }

    // We generate the next few sequence indices in advance (buffering)
    const int maxSeqIdx = m_hoverSequenceIndex + 5;

    QHash<QByteArray, QVariant> data = m_model->data(index);

    if (!data.contains("hoverSequencePixmaps")) {
        // The pixmap at index 0 isn't used ("iconPixmap" will be used instead)
        data.insert("hoverSequencePixmaps", QVariant::fromValue(QVector<QPixmap>() << QPixmap()));
        m_model->setData(index, data);
    }

    const QVector<QPixmap> pixmaps = data["hoverSequencePixmaps"].value<QVector<QPixmap>>();

    const int loadSeqIdx = pixmaps.size();

    float wap = -1.0f;
    if (data.contains("hoverSequenceWraparoundPoint")) {
        wap = data["hoverSequenceWraparoundPoint"].toFloat();
    }
    if (wap >= 1.0f && loadSeqIdx >= static_cast<int>(wap)) {
        // Reached the wraparound point -> no more previews to load.
        return;
    }

    if (loadSeqIdx > maxSeqIdx) {
        // Wait until setHoverSequenceState() is called with a higher sequence index.
        return;
    }

    // PreviewJob internally caches items always with the size of
    // 128 x 128 pixels or 256 x 256 pixels. A (slow) downscaling is done
    // by PreviewJob if a smaller size is requested. For images KFileItemModelRolesUpdater must
    // do a downscaling anyhow because of the frame, so in this case only the provided
    // cache sizes are requested.
    const QSize cacheSize = (m_iconSize.width() > 128) || (m_iconSize.height() > 128) ? QSize(256, 256) : QSize(128, 128);

    KIO::PreviewJob *job = new KIO::PreviewJob({m_hoverSequenceItem}, cacheSize, &m_enabledPlugins);

    job->setSequenceIndex(loadSeqIdx);
    job->setIgnoreMaximumSize(m_hoverSequenceItem.isLocalFile() && !m_hoverSequenceItem.isSlow() && m_localFileSizePreviewLimit <= 0);
    if (job->uiDelegate()) {
        KJobWidgets::setWindow(job, qApp->activeWindow());
    }

    connect(job, &KIO::PreviewJob::gotPreview, this, &KFileItemModelRolesUpdater::slotHoverSequenceGotPreview);
    connect(job, &KIO::PreviewJob::failed, this, &KFileItemModelRolesUpdater::slotHoverSequencePreviewFailed);
    connect(job, &KIO::PreviewJob::finished, this, &KFileItemModelRolesUpdater::slotHoverSequencePreviewJobFinished);

    m_hoverSequencePreviewJob = job;
}

void KFileItemModelRolesUpdater::killHoverSequencePreviewJob()
{
    if (m_hoverSequencePreviewJob) {
        disconnect(m_hoverSequencePreviewJob, &KIO::PreviewJob::gotPreview, this, &KFileItemModelRolesUpdater::slotHoverSequenceGotPreview);
        disconnect(m_hoverSequencePreviewJob, &KIO::PreviewJob::failed, this, &KFileItemModelRolesUpdater::slotHoverSequencePreviewFailed);
        disconnect(m_hoverSequencePreviewJob, &KIO::PreviewJob::finished, this, &KFileItemModelRolesUpdater::slotHoverSequencePreviewJobFinished);
        m_hoverSequencePreviewJob->kill();
        m_hoverSequencePreviewJob = nullptr;
    }
}

void KFileItemModelRolesUpdater::updateChangedItems()
{
    if (m_state == Paused) {
        return;
    }

    if (m_changedItems.isEmpty()) {
        return;
    }

    m_finishedItems -= m_changedItems;

    if (m_resolvableRoles.contains(m_model->sortRole())) {
        m_pendingSortRoleItems += m_changedItems;

        if (m_state != ResolvingSortRole) {
            // Stop the preview job if necessary, and trigger the
            // asynchronous determination of the sort role.
            killPreviewJob();
            m_state = ResolvingSortRole;
            QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::resolveNextSortRole);
        }

        return;
    }

    QList<int> visibleChangedIndexes;
    QList<int> invisibleChangedIndexes;
    visibleChangedIndexes.reserve(m_changedItems.size());
    invisibleChangedIndexes.reserve(m_changedItems.size());

    auto changedItemsIt = m_changedItems.begin();
    while (changedItemsIt != m_changedItems.end()) {
        const auto &item = *changedItemsIt;
        const int index = m_model->index(item);

        if (index < 0) {
            changedItemsIt = m_changedItems.erase(changedItemsIt);
            continue;
        }
        ++changedItemsIt;

        if (index >= m_firstVisibleIndex && index <= m_lastVisibleIndex) {
            visibleChangedIndexes.append(index);
        } else {
            invisibleChangedIndexes.append(index);
        }
    }

    std::sort(visibleChangedIndexes.begin(), visibleChangedIndexes.end());

    if (m_previewShown) {
        for (int index : qAsConst(visibleChangedIndexes)) {
            m_pendingPreviewItems.append(m_model->fileItem(index));
        }

        for (int index : qAsConst(invisibleChangedIndexes)) {
            m_pendingPreviewItems.append(m_model->fileItem(index));
        }

        if (!m_previewJob) {
            startPreviewJob();
        }
    } else {
        const bool resolvingInProgress = !m_pendingIndexes.isEmpty();
        m_pendingIndexes = visibleChangedIndexes + m_pendingIndexes + invisibleChangedIndexes;
        if (!resolvingInProgress) {
            // Trigger the asynchronous resolving of the changed roles.
            m_state = ResolvingAllRoles;
            QTimer::singleShot(0, this, &KFileItemModelRolesUpdater::resolveNextPendingRoles);
        }
    }
}

void KFileItemModelRolesUpdater::applySortRole(int index)
{
    QHash<QByteArray, QVariant> data;
    const KFileItem item = m_model->fileItem(index);

    if (m_model->sortRole() == "type") {
        if (!item.isMimeTypeKnown()) {
            item.determineMimeType();
        }

        data.insert("type", item.mimeComment());
    } else if (m_model->sortRole() == "size" && item.isLocalFile() && item.isDir()) {
        const QString path = item.localPath();
        if (m_scanDirectories) {
            m_directoryContentsCounter->scanDirectory(path);
        }
    } else {
        // Probably the sort role is a baloo role - just determine all roles.
        data = rolesData(item);
    }

    disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
    m_model->setData(index, data);
    connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
}

void KFileItemModelRolesUpdater::applySortProgressToModel()
{
    // Inform the model about the progress of the resolved items,
    // so that it can give an indication when the sorting has been finished.
    const int resolvedCount = m_model->count() - m_pendingSortRoleItems.count();
    m_model->emitSortProgress(resolvedCount);
}

bool KFileItemModelRolesUpdater::applyResolvedRoles(int index, ResolveHint hint)
{
    const KFileItem item = m_model->fileItem(index);
    const bool resolveAll = (hint == ResolveAll);

    bool iconChanged = false;
    if (!item.isMimeTypeKnown() || !item.isFinalIconKnown()) {
        item.determineMimeType();
        iconChanged = true;
    } else if (!m_model->data(index).contains("iconName")) {
        iconChanged = true;
    }

    if (iconChanged || resolveAll || m_clearPreviews) {
        if (index < 0) {
            return false;
        }

        QHash<QByteArray, QVariant> data;
        if (resolveAll) {
            data = rolesData(item);
        }

        if (!item.iconName().isEmpty()) {
            data.insert("iconName", item.iconName());
        }

        if (m_clearPreviews) {
            data.insert("iconPixmap", QPixmap());
            data.insert("hoverSequencePixmaps", QVariant::fromValue(QVector<QPixmap>()));
        }

        disconnect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
        m_model->setData(index, data);
        connect(m_model, &KFileItemModel::itemsChanged, this, &KFileItemModelRolesUpdater::slotItemsChanged);
        return true;
    }

    return false;
}

QHash<QByteArray, QVariant> KFileItemModelRolesUpdater::rolesData(const KFileItem &item)
{
    QHash<QByteArray, QVariant> data;

    const bool getSizeRole = m_roles.contains("size");
    const bool getIsExpandableRole = m_roles.contains("isExpandable");

    if ((getSizeRole || getIsExpandableRole) && item.isDir()) {
        if (item.isLocalFile()) {
            // Tell m_directoryContentsCounter that we want to count the items
            // inside the directory. The result will be received in slotDirectoryContentsCountReceived.
            if (m_scanDirectories) {
                const QString path = item.localPath();
                m_directoryContentsCounter->scanDirectory(path);
            }
        } else if (getSizeRole) {
            data.insert("size", -1); // -1 indicates an unknown number of items
        }
    }

    if (m_roles.contains("extension")) {
        data.insert("extension", QFileInfo(item.name()).suffix());
    }

    if (m_roles.contains("type")) {
        data.insert("type", item.mimeComment());
    }

    QStringList overlays = item.overlays();
    for (KOverlayIconPlugin *it : qAsConst(m_overlayIconsPlugin)) {
        overlays.append(it->getOverlays(item.url()));
    }
    if (!overlays.isEmpty()) {
        data.insert("iconOverlays", overlays);
    }

#if HAVE_BALOO
    if (m_balooFileMonitor) {
        m_balooFileMonitor->addFile(item.localPath());
        applyChangedBalooRolesForItem(item);
    }
#endif
    return data;
}

void KFileItemModelRolesUpdater::slotOverlaysChanged(const QUrl &url, const QStringList &)
{
    const KFileItem item = m_model->fileItem(url);
    if (item.isNull()) {
        return;
    }
    const int index = m_model->index(item);
    QHash<QByteArray, QVariant> data = m_model->data(index);
    QStringList overlays = item.overlays();
    for (KOverlayIconPlugin *it : qAsConst(m_overlayIconsPlugin)) {
        overlays.append(it->getOverlays(url));
    }
    data.insert("iconOverlays", overlays);
    m_model->setData(index, data);
}

void KFileItemModelRolesUpdater::updateAllPreviews()
{
    if (m_state == Paused) {
        m_previewChangedDuringPausing = true;
    } else {
        m_finishedItems.clear();
        startUpdating();
    }
}

void KFileItemModelRolesUpdater::killPreviewJob()
{
    if (m_previewJob) {
        disconnect(m_previewJob, &KIO::PreviewJob::gotPreview, this, &KFileItemModelRolesUpdater::slotGotPreview);
        disconnect(m_previewJob, &KIO::PreviewJob::failed, this, &KFileItemModelRolesUpdater::slotPreviewFailed);
        disconnect(m_previewJob, &KIO::PreviewJob::finished, this, &KFileItemModelRolesUpdater::slotPreviewJobFinished);
        m_previewJob->kill();
        m_previewJob = nullptr;
        m_pendingPreviewItems.clear();
    }
}

QList<int> KFileItemModelRolesUpdater::indexesToResolve() const
{
    const int count = m_model->count();

    QList<int> result;
    result.reserve(qMin(count, (m_lastVisibleIndex - m_firstVisibleIndex + 1) + ResolveAllItemsLimit + (2 * m_maximumVisibleItems)));

    // Add visible items.
    // Resolve files first, their previews are quicker.
    QList<int> visibleDirs;
    for (int i = m_firstVisibleIndex; i <= m_lastVisibleIndex; ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (item.isDir()) {
            visibleDirs.append(i);
        } else {
            result.append(i);
        }
    }

    result.append(visibleDirs);

    // We need a reasonable upper limit for number of items to resolve after
    // and before the visible range. m_maximumVisibleItems can be quite large
    // when using Compact View.
    const int readAheadItems = qMin(ReadAheadPages * m_maximumVisibleItems, ResolveAllItemsLimit / 2);

    // Add items after the visible range.
    const int endExtendedVisibleRange = qMin(m_lastVisibleIndex + readAheadItems, count - 1);
    for (int i = m_lastVisibleIndex + 1; i <= endExtendedVisibleRange; ++i) {
        result.append(i);
    }

    // Add items before the visible range in reverse order.
    const int beginExtendedVisibleRange = qMax(0, m_firstVisibleIndex - readAheadItems);
    for (int i = m_firstVisibleIndex - 1; i >= beginExtendedVisibleRange; --i) {
        result.append(i);
    }

    // Add items on the last page.
    const int beginLastPage = qMax(endExtendedVisibleRange + 1, count - m_maximumVisibleItems);
    for (int i = beginLastPage; i < count; ++i) {
        result.append(i);
    }

    // Add items on the first page.
    const int endFirstPage = qMin(beginExtendedVisibleRange, m_maximumVisibleItems);
    for (int i = 0; i < endFirstPage; ++i) {
        result.append(i);
    }

    // Continue adding items until ResolveAllItemsLimit is reached.
    int remainingItems = ResolveAllItemsLimit - result.count();

    for (int i = endExtendedVisibleRange + 1; i < beginLastPage && remainingItems > 0; ++i) {
        result.append(i);
        --remainingItems;
    }

    for (int i = beginExtendedVisibleRange - 1; i >= endFirstPage && remainingItems > 0; --i) {
        result.append(i);
        --remainingItems;
    }

    return result;
}

void KFileItemModelRolesUpdater::trimHoverSequenceLoadedItems()
{
    static const size_t maxLoadedItems = 20;

    size_t loadedItems = m_hoverSequenceLoadedItems.size();
    while (loadedItems > maxLoadedItems) {
        const KFileItem item = m_hoverSequenceLoadedItems.front();

        m_hoverSequenceLoadedItems.pop_front();
        loadedItems--;

        const int index = m_model->index(item);
        if (index >= 0) {
            QHash<QByteArray, QVariant> data = m_model->data(index);
            data["hoverSequencePixmaps"] = QVariant::fromValue(QVector<QPixmap>() << QPixmap());
            m_model->setData(index, data);
        }
    }
}
