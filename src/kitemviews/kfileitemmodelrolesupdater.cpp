/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kfileitemmodelrolesupdater.h"

#include "kfileitemmodel.h"
#include "kpixmapmodifier_p.h"

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KFileItem>
#include <KGlobal>
#include <KIO/PreviewJob>
#include <QPainter>
#include <QPixmap>
#include <QElapsedTimer>
#include <QTimer>

// Required includes for subDirectoriesCount():
#ifdef Q_WS_WIN
    #include <QDir>
#else
    #include <dirent.h>
    #include <QFile>
#endif

#define KFILEITEMMODELROLESUPDATER_DEBUG

namespace {
    // Maximum time in ms that the KFileItemModelRolesUpdater
    // may perform a blocking operation
    const int MaxBlockTimeout = 200;

    // Maximum number of items that will get resolved synchronously.
    // The value should roughly represent the number of maximum visible
    // items, as it does not make sense to resolve more items synchronously
    // and probably reach the MaxBlockTimeout because of invisible items.
    const int MaxResolveItemsCount = 100;
}

KFileItemModelRolesUpdater::KFileItemModelRolesUpdater(KFileItemModel* model, QObject* parent) :
    QObject(parent),
    m_paused(false),
    m_previewChangedDuringPausing(false),
    m_iconSizeChangedDuringPausing(false),
    m_rolesChangedDuringPausing(false),
    m_previewShown(false),
    m_clearPreviews(false),
    m_model(model),
    m_iconSize(),
    m_firstVisibleIndex(0),
    m_lastVisibleIndex(-1),
    m_roles(),
    m_enabledPlugins(),
    m_pendingVisibleItems(),
    m_pendingInvisibleItems(),
    m_previewJobs(),
    m_resolvePendingRolesTimer(0)
{
    Q_ASSERT(model);

    const KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
    m_enabledPlugins = globalConfig.readEntry("Plugins", QStringList()
                                                         << "directorythumbnail"
                                                         << "imagethumbnail"
                                                         << "jpegthumbnail");

    connect(m_model, SIGNAL(itemsInserted(KItemRangeList)),
            this,    SLOT(slotItemsInserted(KItemRangeList)));
    connect(m_model, SIGNAL(itemsRemoved(KItemRangeList)),
            this,    SLOT(slotItemsRemoved(KItemRangeList)));
    connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
            this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));

    // A timer with a minimal timeout is used to merge several triggerPendingRolesResolving() calls
    // to only one call of resolvePendingRoles().
    m_resolvePendingRolesTimer = new QTimer(this);
    m_resolvePendingRolesTimer->setInterval(1);
    m_resolvePendingRolesTimer->setSingleShot(true);
    connect(m_resolvePendingRolesTimer, SIGNAL(timeout()), this, SLOT(resolvePendingRoles()));
}

KFileItemModelRolesUpdater::~KFileItemModelRolesUpdater()
{
}

void KFileItemModelRolesUpdater::setIconSize(const QSize& size)
{
    if (size != m_iconSize) {
        m_iconSize = size;
        if (m_paused) {
            m_iconSizeChangedDuringPausing = true;
        } else if (m_previewShown) {
            // An icon size change requires the regenerating of
            // all previews
            sortAndResolveAllRoles();
        } else {
            sortAndResolvePendingRoles();
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

    if (hasPendingRoles() && !m_paused) {
        sortAndResolvePendingRoles();
    }
}

void KFileItemModelRolesUpdater::setPreviewShown(bool show)
{
    if (show == m_previewShown) {
        return;
    }

    m_previewShown = show;
    if (!show) {
        m_clearPreviews = true;
    }

    if (m_paused) {
        m_previewChangedDuringPausing = true;
    } else {
        sortAndResolveAllRoles();
    }
}

bool KFileItemModelRolesUpdater::isPreviewShown() const
{
    return m_previewShown;
}

void KFileItemModelRolesUpdater::setEnabledPlugins(const QStringList& list)
{
    m_enabledPlugins = list;
}

void KFileItemModelRolesUpdater::setPaused(bool paused)
{
    if (paused == m_paused) {
        return;
    }

    m_paused = paused;
    if (paused) {
        if (hasPendingRoles()) {
            foreach (KJob* job, m_previewJobs) {
                job->kill();
            }
            Q_ASSERT(m_previewJobs.isEmpty());
        }
    } else {
        const bool resolveAll = (m_iconSizeChangedDuringPausing && m_previewShown) ||
                                m_previewChangedDuringPausing ||
                                m_rolesChangedDuringPausing;
        if (resolveAll) {
            sortAndResolveAllRoles();
        } else {
            sortAndResolvePendingRoles();
        }

        m_iconSizeChangedDuringPausing = false;
        m_previewChangedDuringPausing = false;
        m_rolesChangedDuringPausing = false;
    }
}

void KFileItemModelRolesUpdater::setRoles(const QSet<QByteArray>& roles)
{
    if (roles.count() == m_roles.count()) {
        bool isEqual = true;
        foreach (const QByteArray& role, roles) {
            if (!m_roles.contains(role)) {
                isEqual = false;
                break;
            }
        }
        if (isEqual) {
            return;
        }
    }

    m_roles = roles;

    if (m_paused) {
        m_rolesChangedDuringPausing = true;
    } else {
        sortAndResolveAllRoles();
    }
}

QSet<QByteArray> KFileItemModelRolesUpdater::roles() const
{
    return m_roles;
}

bool KFileItemModelRolesUpdater::isPaused() const
{
    return m_paused;
}

QStringList KFileItemModelRolesUpdater::enabledPlugins() const
{
    return m_enabledPlugins;
}

void KFileItemModelRolesUpdater::slotItemsInserted(const KItemRangeList& itemRanges)
{
    // If no valid index range is given assume that all items are visible.
    // A cleanup will be done later as soon as the index range has been set.
    const bool hasValidIndexRange = (m_lastVisibleIndex >= 0);

    if (hasValidIndexRange) {
        // Move all current pending visible items that are not visible anymore
        // to the pending invisible items.
        QSetIterator<KFileItem> it(m_pendingVisibleItems);
        while (it.hasNext()) {
            const KFileItem item = it.next();
            const int index = m_model->index(item);
            if (index < m_firstVisibleIndex || index > m_lastVisibleIndex) {
                m_pendingVisibleItems.remove(item);
                m_pendingInvisibleItems.insert(item);
            }
        }
    }

    int rangesCount = 0;

    foreach (const KItemRange& range, itemRanges) {
        rangesCount += range.count;

        // Add the inserted items to the pending visible and invisible items
        const int lastIndex = range.index + range.count - 1;
        for (int i = range.index; i <= lastIndex; ++i) {
            const KFileItem item = m_model->fileItem(i);
            if (!hasValidIndexRange || (i >= m_firstVisibleIndex && i <= m_lastVisibleIndex)) {
                m_pendingVisibleItems.insert(item);
            } else {
                m_pendingInvisibleItems.insert(item);
            }
        }
    }

    triggerPendingRolesResolving(rangesCount);
}

void KFileItemModelRolesUpdater::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    Q_UNUSED(itemRanges);
    m_firstVisibleIndex = 0;
    m_lastVisibleIndex = -1;    
    if (!hasPendingRoles()) {
        return;
    }

    if (m_model->count() == 0) {
        // Most probably a directory change is done. Clear all pending items
        // and also kill all ongoing preview-jobs.
        resetPendingRoles();
    } else {
        // Remove all items from m_pendingVisibleItems and m_pendingInvisibleItems
        // that are not part of the model anymore.
        for (int i = 0; i <= 1; ++i) {
            QSet<KFileItem>& pendingItems = (i == 0) ? m_pendingVisibleItems : m_pendingInvisibleItems;
            QMutableSetIterator<KFileItem> it(pendingItems);
            while (it.hasNext()) {
                const KFileItem item = it.next();
                if (m_model->index(item) < 0) {
                    pendingItems.remove(item);
                }
            }
        }
    }
}

void KFileItemModelRolesUpdater::slotItemsChanged(const KItemRangeList& itemRanges,
                                                  const QSet<QByteArray>& roles)
{
    Q_UNUSED(itemRanges);
    Q_UNUSED(roles);
    // TODO
}

void KFileItemModelRolesUpdater::slotGotPreview(const KFileItem& item, const QPixmap& pixmap)
{
    m_pendingVisibleItems.remove(item);
    m_pendingInvisibleItems.remove(item);

    const int index = m_model->index(item);
    if (index < 0) {
        return;
    }

    QPixmap scaledPixmap = pixmap;

    const QString mimeType = item.mimetype();
    const int slashIndex = mimeType.indexOf(QLatin1Char('/'));
    const QString mimeTypeGroup = mimeType.left(slashIndex);
    if (mimeTypeGroup == QLatin1String("image")) {
        KPixmapModifier::applyFrame(scaledPixmap, m_iconSize);
    } else {
        KPixmapModifier::scale(scaledPixmap, m_iconSize);
    }

    QHash<QByteArray, QVariant> data = rolesData(item);
    data.insert("iconPixmap", scaledPixmap);

    disconnect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
               this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
    m_model->setData(index, data);
    connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
            this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
}

void KFileItemModelRolesUpdater::slotPreviewFailed(const KFileItem& item)
{
    m_pendingVisibleItems.remove(item);
    m_pendingInvisibleItems.remove(item);

    const bool clearPreviews = m_clearPreviews;
    m_clearPreviews = true;
    applyResolvedRoles(item, ResolveAll);
    m_clearPreviews = clearPreviews;
}

void KFileItemModelRolesUpdater::slotPreviewJobFinished(KJob* job)
{
#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
    kDebug() << "Preview job finished. Pending visible:" << m_pendingVisibleItems.count() << "invisible:" << m_pendingInvisibleItems.count();
#endif

    m_previewJobs.removeOne(job);
    if (!m_previewJobs.isEmpty() || !hasPendingRoles()) {
        return;
    }

    const KFileItemList visibleItems = sortedItems(m_pendingVisibleItems);
    startPreviewJob(visibleItems + m_pendingInvisibleItems.toList());
}

void KFileItemModelRolesUpdater::resolvePendingRoles()
{
    int resolvedCount = 0;

    const bool hasSlowRoles = m_previewShown
                              || m_roles.contains("size")
                              || m_roles.contains("type");
    const ResolveHint resolveHint = hasSlowRoles ? ResolveFast : ResolveAll;

    // Resolving the MIME type can be expensive. Assure that not more than MaxBlockTimeout ms are
    // spend for resolving them synchronously. Usually this is more than enough to determine
    // all visible items, but there are corner cases where this limit gets easily exceeded.
    QElapsedTimer timer;
    timer.start();

    // Resolve the MIME type of all visible items
    QSetIterator<KFileItem> visibleIt(m_pendingVisibleItems);
    while (visibleIt.hasNext()) {
        const KFileItem item = visibleIt.next();
        applyResolvedRoles(item, resolveHint);
        if (!hasSlowRoles) {
            Q_ASSERT(!m_pendingInvisibleItems.contains(item));
            // All roles have been resolved already by applyResolvedRoles()
            m_pendingVisibleItems.remove(item);
        }
        ++resolvedCount;

        if (timer.elapsed() > MaxBlockTimeout) {
            break;
        }
    }

    // Resolve the MIME type of the invisible items at least until the timeout
    // has been exceeded or the maximum number of items has been reached
    KFileItemList invisibleItems;
    if (m_lastVisibleIndex >= 0) {
        // The visible range is valid, don't care about the order how the MIME
        // type of invisible items get resolved
        invisibleItems = m_pendingInvisibleItems.toList();
    } else {
        // The visible range is temporary invalid (e.g. happens when loading
        // a directory) so take care to sort the currently invisible items where
        // a part will get visible later
        invisibleItems = sortedItems(m_pendingInvisibleItems);
    }

    int index = 0;
    while (resolvedCount < MaxResolveItemsCount && index < invisibleItems.count() && timer.elapsed() <= MaxBlockTimeout) {
        const KFileItem item = invisibleItems.at(index);
        applyResolvedRoles(item, resolveHint);

        if (!hasSlowRoles) {
            // All roles have been resolved already by applyResolvedRoles()
            m_pendingInvisibleItems.remove(item);
        }
        ++index;
        ++resolvedCount;
    }

    if (m_previewShown) {
        KFileItemList items = sortedItems(m_pendingVisibleItems);
        items += invisibleItems;
        startPreviewJob(items);
    } else {
        QTimer::singleShot(0, this, SLOT(resolveNextPendingRoles()));
    }

#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
    if (timer.elapsed() > MaxBlockTimeout) {
        kDebug() << "Maximum time of" << MaxBlockTimeout
                 << "ms exceeded, skipping items... Remaining visible:" << m_pendingVisibleItems.count()
                 << "invisible:" << m_pendingInvisibleItems.count();
    }
    kDebug() << "[TIME] Resolved pending roles:" << timer.elapsed();
#endif
}

void KFileItemModelRolesUpdater::resolveNextPendingRoles()
{
    if (m_paused) {
        return;
    }

    if (m_previewShown) {
        // The preview has been turned on since the last run. Skip
        // resolving further pending roles as this is done as soon
        // as a preview has been received.
        return;
    }

    int resolvedCount = 0;
    bool changed = false;
    for (int i = 0; i <= 1; ++i) {
        QSet<KFileItem>& pendingItems = (i == 0) ? m_pendingVisibleItems : m_pendingInvisibleItems;
        QSetIterator<KFileItem> it(pendingItems);
        while (it.hasNext() && !changed && resolvedCount < MaxResolveItemsCount) {
            const KFileItem item = it.next();
            pendingItems.remove(item);
            changed = applyResolvedRoles(item, ResolveAll);
            ++resolvedCount;
        }
    }

    if (hasPendingRoles()) {
        QTimer::singleShot(0, this, SLOT(resolveNextPendingRoles()));
    } else {
        m_clearPreviews = false;
    }

#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
    static int callCount = 0;
    ++callCount;
    if (callCount % 100 == 0) {
        kDebug() << "Remaining visible roles to resolve:" << m_pendingVisibleItems.count()
                 << "invisible:" << m_pendingInvisibleItems.count();
    }
#endif
}

void KFileItemModelRolesUpdater::startPreviewJob(const KFileItemList& items)
{
    if (items.isEmpty() || m_paused) {
        return;
    }

    // PreviewJob internally caches items always with the size of
    // 128 x 128 pixels or 256 x 256 pixels. A (slow) downscaling is done
    // by PreviewJob if a smaller size is requested. For images KFileItemModelRolesUpdater must
    // do a downscaling anyhow because of the frame, so in this case only the provided
    // cache sizes are requested.
    const QSize cacheSize = (m_iconSize.width() > 128) || (m_iconSize.height() > 128)
                            ? QSize(256, 256) : QSize(128, 128);

    // KIO::filePreview() will request the MIME-type of all passed items, which (in the
    // worst case) might block the application for several seconds. To prevent such
    // a blocking the MIME-type of the items will determined until the MaxBlockTimeout
    // has been reached and only those items will get passed. As soon as the MIME-type
    // has been resolved once KIO::filePreview() can already access the resolved
    // MIME-type in a fast way.
    QElapsedTimer timer;
    timer.start();
    KFileItemList itemSubSet;
    for (int i = 0; i < items.count(); ++i) {
        KFileItem item = items.at(i);
        item.determineMimeType();
        itemSubSet.append(items.at(i));
        if (timer.elapsed() > MaxBlockTimeout) {
#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
            kDebug() << "Maximum time of" << MaxBlockTimeout << "ms exceeded, creating only previews for"
                     << (i + 1) << "items," << (items.count() - (i + 1)) << "will be resolved later";
#endif
            break;
        }
    }
    KJob* job = KIO::filePreview(itemSubSet, cacheSize, &m_enabledPlugins);

    connect(job,  SIGNAL(gotPreview(KFileItem,QPixmap)),
            this, SLOT(slotGotPreview(KFileItem,QPixmap)));
    connect(job,  SIGNAL(failed(KFileItem)),
            this, SLOT(slotPreviewFailed(KFileItem)));
    connect(job,  SIGNAL(finished(KJob*)),
            this, SLOT(slotPreviewJobFinished(KJob*)));

    m_previewJobs.append(job);
}


bool KFileItemModelRolesUpdater::hasPendingRoles() const
{
    return !m_pendingVisibleItems.isEmpty() || !m_pendingInvisibleItems.isEmpty();
}

void KFileItemModelRolesUpdater::resetPendingRoles()
{
    m_pendingVisibleItems.clear();
    m_pendingInvisibleItems.clear();

    foreach (KJob* job, m_previewJobs) {
        job->kill();
    }
    Q_ASSERT(m_previewJobs.isEmpty());
}

void KFileItemModelRolesUpdater::triggerPendingRolesResolving(int count)
{
    if (count == m_model->count()) {
        // When initially loading a directory a synchronous resolving prevents a minor
        // flickering when opening directories. This is also fine from a performance point
        // of view as it is assured in resolvePendingRoles() to never block the event-loop
        // for more than 200 ms.
        resolvePendingRoles();
    } else {
        // Items have been added. This can be done in several small steps within one loop
        // because of the sorting and hence may not trigger any expensive operation.
        m_resolvePendingRolesTimer->start();
    }
}

void KFileItemModelRolesUpdater::sortAndResolveAllRoles()
{
    if (m_paused) {
        return;
    }

    resetPendingRoles();
    Q_ASSERT(m_pendingVisibleItems.isEmpty());
    Q_ASSERT(m_pendingInvisibleItems.isEmpty());

    if (m_model->count() == 0) {
        return;
    }

    // Determine all visible items
    Q_ASSERT(m_firstVisibleIndex >= 0);
    for (int i = m_firstVisibleIndex; i <= m_lastVisibleIndex; ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (!item.isNull()) {
            m_pendingVisibleItems.insert(item);
        }
    }

    // Determine all invisible items
    for (int i = 0; i < m_firstVisibleIndex; ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (!item.isNull()) {
            m_pendingInvisibleItems.insert(item);
        }
    }
    for (int i = m_lastVisibleIndex + 1; i < m_model->count(); ++i) {
        const KFileItem item = m_model->fileItem(i);
        if (!item.isNull()) {
            m_pendingInvisibleItems.insert(item);
        }
    }

    triggerPendingRolesResolving(m_pendingVisibleItems.count() +
                                 m_pendingInvisibleItems.count());
}

void KFileItemModelRolesUpdater::sortAndResolvePendingRoles()
{
    Q_ASSERT(!m_paused);
    if (m_model->count() == 0) {
        return;
    }

    // If no valid index range is given assume that all items are visible.
    // A cleanup will be done later as soon as the index range has been set.
    const bool hasValidIndexRange = (m_lastVisibleIndex >= 0);

    // Trigger a preview generation of all pending items. Assure that the visible
    // pending items get generated first.
    QSet<KFileItem> pendingItems;
    pendingItems += m_pendingVisibleItems;
    pendingItems += m_pendingInvisibleItems;

    resetPendingRoles();
    Q_ASSERT(m_pendingVisibleItems.isEmpty());
    Q_ASSERT(m_pendingInvisibleItems.isEmpty());

    QSetIterator<KFileItem> it(pendingItems);
    while (it.hasNext()) {
        const KFileItem item = it.next();
        if (item.isNull()) {
            continue;
        }

        const int index = m_model->index(item);
        if (!hasValidIndexRange || (index >= m_firstVisibleIndex && index <= m_lastVisibleIndex)) {
            m_pendingVisibleItems.insert(item);
        } else {
            m_pendingInvisibleItems.insert(item);
        }
    }

    triggerPendingRolesResolving(m_pendingVisibleItems.count() +
                                 m_pendingInvisibleItems.count());
}

bool KFileItemModelRolesUpdater::applyResolvedRoles(const KFileItem& item, ResolveHint hint)
{
    if (item.isNull()) {
        return false;
    }

    const bool resolveAll = (hint == ResolveAll);

    bool mimeTypeChanged = false;
    if (!item.isMimeTypeKnown()) {
        item.determineMimeType();
        mimeTypeChanged = true;
    }

    if (mimeTypeChanged || resolveAll || m_clearPreviews) {
        const int index = m_model->index(item);
        if (index < 0) {
            return false;
        }

        QHash<QByteArray, QVariant> data;
        if (resolveAll) {
            data = rolesData(item);
        }

        data.insert("iconName", item.iconName());

        if (m_clearPreviews) {
            data.insert("iconPixmap", QString());
        }

        disconnect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
                   this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
        m_model->setData(index, data);
        connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
                this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
        return true;
    }

    return false;
}

QHash<QByteArray, QVariant> KFileItemModelRolesUpdater::rolesData(const KFileItem& item) const
{
    QHash<QByteArray, QVariant> data;

    if (m_roles.contains("size")) {
        if (item.isDir() && item.isLocalFile()) {
            const QString path = item.localPath();
            const int count = subDirectoriesCount(path);
            if (count >= 0) {
                data.insert("size", KIO::filesize_t(count));
            }
        }
    }

    if (m_roles.contains("type")) {
        data.insert("type", item.mimeComment());
    }

    data.insert("iconOverlays", item.overlays());

    return data;
}

KFileItemList KFileItemModelRolesUpdater::sortedItems(const QSet<KFileItem>& items) const
{
    KFileItemList itemList;
    if (items.isEmpty()) {
        return itemList;
    }

#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    QList<int> indexes;
    indexes.reserve(items.count());

    QSetIterator<KFileItem> it(items);
    while (it.hasNext()) {
        const KFileItem item = it.next();
        const int index = m_model->index(item);
        if (index >= 0) {
            indexes.append(index);
        }
    }
    qSort(indexes);

    itemList.reserve(items.count());
    foreach (int index, indexes) {
        itemList.append(m_model->fileItem(index));
    }

#ifdef KFILEITEMMODELROLESUPDATER_DEBUG
    kDebug() << "[TIME] Sorting of items:" << timer.elapsed();
#endif
    return itemList;
}

int KFileItemModelRolesUpdater::subDirectoriesCount(const QString& path)
{
#ifdef Q_WS_WIN
    QDir dir(path);
    return dir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::System).count();
#else
    // Taken from kdelibs/kio/kio/kdirmodel.cpp
    // Copyright (C) 2006 David Faure <faure@kde.org>

    int count = -1;
    DIR* dir = ::opendir(QFile::encodeName(path));
    if (dir) {
        count = 0;
        struct dirent *dirEntry = 0;
        while ((dirEntry = ::readdir(dir))) { // krazy:exclude=syscalls
            if (dirEntry->d_name[0] == '.') {
                if (dirEntry->d_name[1] == '\0') {
                    // Skip "."
                    continue;
                }
                if (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0') {
                    // Skip ".."
                    continue;
                }
            }
            ++count;
        }
        ::closedir(dir);
    }
    return count;
#endif
}

#include "kfileitemmodelrolesupdater.moc"
