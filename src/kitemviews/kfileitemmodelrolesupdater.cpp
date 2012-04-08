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

#ifdef HAVE_NEPOMUK
    #include "knepomukrolesprovider_p.h"
    #include "knepomukresourcewatcher_p.h"
#endif

// Required includes for subItemsCount():
#ifdef Q_WS_WIN
    #include <QDir>
#else
    #include <dirent.h>
    #include <QFile>
#endif

// #define KFILEITEMMODELROLESUPDATER_DEBUG

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
    m_enlargeSmallPreviews(true),
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
    m_changedItemsTimer(0),
    m_changedItems()
  #ifdef HAVE_NEPOMUK
  , m_nepomukResourceWatcher(0),
    m_nepomukUriItems()
  #endif

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

    // Use a timer to prevent that each call of slotItemsChanged() results in a synchronous
    // resolving of the roles. Postpone the resolving until no update has been done for 1 second.
    m_changedItemsTimer = new QTimer(this);
    m_changedItemsTimer->setInterval(1000);
    m_changedItemsTimer->setSingleShot(true);
    connect(m_changedItemsTimer, SIGNAL(timeout()), this, SLOT(resolveChangedItems()));
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

void KFileItemModelRolesUpdater::setEnabledPlugins(const QStringList& list)
{
    if (m_enabledPlugins == list) {
        m_enabledPlugins = list;
        if (m_previewShown) {
            updateAllPreviews();
        }
    }
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
    if (m_roles != roles) {
        m_roles = roles;

#ifdef HAVE_NEPOMUK
        // Check whether there is at least one role that must be resolved
        // with the help of Nepomuk. If this is the case, a (quite expensive)
        // resolving will be done in KFileItemModelRolesUpdater::rolesData() and
        // the role gets watched for changes.
        const KNepomukRolesProvider& rolesProvider = KNepomukRolesProvider::instance();
        bool hasNepomukRole = false;
        QSetIterator<QByteArray> it(roles);
        while (it.hasNext()) {
            const QByteArray& role = it.next();
            if (rolesProvider.isNepomukRole(role)) {
                hasNepomukRole = true;
                break;
            }
        }

        if (hasNepomukRole && !m_nepomukResourceWatcher) {
            Q_ASSERT(m_nepomukUriItems.isEmpty());

            m_nepomukResourceWatcher = new Nepomuk::ResourceWatcher(this);
            connect(m_nepomukResourceWatcher, SIGNAL(propertyChanged(Nepomuk::Resource,Nepomuk::Types::Property,QVariantList,QVariantList)),
                    this, SLOT(applyChangedNepomukRoles(Nepomuk::Resource)));
            connect(m_nepomukResourceWatcher, SIGNAL(propertyRemoved(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)),
                    this, SLOT(applyChangedNepomukRoles(Nepomuk::Resource)));
            connect(m_nepomukResourceWatcher, SIGNAL(propertyAdded(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)),
                    this, SLOT(applyChangedNepomukRoles(Nepomuk::Resource)));
            connect(m_nepomukResourceWatcher, SIGNAL(resourceCreated(Nepomuk::Resource,QList<QUrl>)),
                    this, SLOT(applyChangedNepomukRoles(Nepomuk::Resource)));
        } else if (!hasNepomukRole && m_nepomukResourceWatcher) {
            delete m_nepomukResourceWatcher;
            m_nepomukResourceWatcher = 0;
            m_nepomukUriItems.clear();
        }
#endif

        if (m_paused) {
            m_rolesChangedDuringPausing = true;
        } else {
            sortAndResolveAllRoles();
        }
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
    startUpdating(itemRanges);
}

void KFileItemModelRolesUpdater::slotItemsRemoved(const KItemRangeList& itemRanges)
{
    Q_UNUSED(itemRanges);

#ifdef HAVE_NEPOMUK
    if (m_nepomukResourceWatcher) {
        // Don't let the ResourceWatcher watch for removed items
        if (m_model->count() == 0) {
            m_nepomukResourceWatcher->setResources(QList<Nepomuk::Resource>());
            m_nepomukUriItems.clear();
        } else {
            QList<Nepomuk::Resource> newResources;
            const QList<Nepomuk::Resource> oldResources = m_nepomukResourceWatcher->resources();
            foreach (const Nepomuk::Resource& resource, oldResources) {
                const QUrl uri = resource.resourceUri();
                const KUrl itemUrl = m_nepomukUriItems.value(uri);
                if (m_model->index(itemUrl) >= 0) {
                    newResources.append(resource);
                } else {
                    m_nepomukUriItems.remove(uri);
                }
            }
            m_nepomukResourceWatcher->setResources(newResources);
        }
    }
#endif

    m_firstVisibleIndex = 0;
    m_lastVisibleIndex = -1;
    if (!hasPendingRoles()) {
        return;
    }

    if (m_model->count() == 0) {
        // Most probably a directory change is done. Clear all pending items
        // and also kill all ongoing preview-jobs.
        resetPendingRoles();

        m_changedItems.clear();
        m_changedItemsTimer->stop();
    } else {
        // Remove all items from m_pendingVisibleItems and m_pendingInvisibleItems
        // that are not part of the model anymore. The items from m_changedItems
        // don't need to be handled here, removed items are just skipped in
        // resolveChangedItems().
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
    Q_UNUSED(roles);

    if (m_changedItemsTimer->isActive()) {
        // A call of slotItemsChanged() has been done recently. Postpone the resolving
        // of the roles until the timer has exceeded.
        foreach (const KItemRange& itemRange, itemRanges) {
            int index = itemRange.index;
            for (int count = itemRange.count; count > 0; --count) {
                m_changedItems.insert(m_model->fileItem(index));
                ++index;
            }
        }
    } else {
        // No call of slotItemsChanged() has been done recently, resolve the roles now.
        startUpdating(itemRanges);
    }
    m_changedItemsTimer->start();
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
        if (m_enlargeSmallPreviews) {
            KPixmapModifier::applyFrame(scaledPixmap, m_iconSize);
        } else {
            // Assure that small previews don't get enlarged. Instead they
            // should be shown centered within the frame.
            const QSize contentSize = KPixmapModifier::sizeInsideFrame(m_iconSize);
            const bool enlargingRequired = scaledPixmap.width()  < contentSize.width() &&
                                           scaledPixmap.height() < contentSize.height();
            if (enlargingRequired) {
                QSize frameSize = scaledPixmap.size();
                frameSize.scale(m_iconSize, Qt::KeepAspectRatio);

                QPixmap largeFrame(frameSize);
                largeFrame.fill(Qt::transparent);

                KPixmapModifier::applyFrame(largeFrame, frameSize);

                QPainter painter(&largeFrame);
                painter.drawPixmap((largeFrame.width()  - scaledPixmap.width()) / 2,
                                   (largeFrame.height() - scaledPixmap.height()) / 2,
                                   scaledPixmap);
                scaledPixmap = largeFrame;
            } else {
                // The image must be shrinked as it is too large to fit into
                // the available icon size
                KPixmapModifier::applyFrame(scaledPixmap, m_iconSize);
            }
        }
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

void KFileItemModelRolesUpdater::resolveChangedItems()
{
    if (m_changedItems.isEmpty()) {
        return;
    }

    KItemRangeList itemRanges;

    QSetIterator<KFileItem> it(m_changedItems);
    while (it.hasNext()) {
        const KFileItem& item = it.next();
        const int index = m_model->index(item);
        if (index >= 0) {
            itemRanges.append(KItemRange(index, 1));
        }
    }
    m_changedItems.clear();

    startUpdating(itemRanges);
}

void KFileItemModelRolesUpdater::applyChangedNepomukRoles(const Nepomuk::Resource& resource)
{
#ifdef HAVE_NEPOMUK
    const KUrl itemUrl = m_nepomukUriItems.value(resource.resourceUri());
    const KFileItem item = m_model->fileItem(itemUrl);
    QHash<QByteArray, QVariant> data = rolesData(item);

    const KNepomukRolesProvider& rolesProvider = KNepomukRolesProvider::instance();
    QHashIterator<QByteArray, QVariant> it(rolesProvider.roleValues(resource, m_roles));
    while (it.hasNext()) {
        it.next();
        data.insert(it.key(), it.value());
    }

    disconnect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
               this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
    const int index = m_model->index(item);
    m_model->setData(index, data);
    connect(m_model, SIGNAL(itemsChanged(KItemRangeList,QSet<QByteArray>)),
            this,    SLOT(slotItemsChanged(KItemRangeList,QSet<QByteArray>)));
#else
    Q_UNUSED(resource);
#endif
}

void KFileItemModelRolesUpdater::startUpdating(const KItemRangeList& itemRanges)
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

    resolvePendingRoles();
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

void KFileItemModelRolesUpdater::resolvePendingRoles()
{
    int resolvedCount = 0;

    const bool hasSlowRoles = m_previewShown
                              || m_roles.contains("size")
                              || m_roles.contains("type")
                              || m_roles.contains("isExpandable");
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

void KFileItemModelRolesUpdater::resetPendingRoles()
{
    m_pendingVisibleItems.clear();
    m_pendingInvisibleItems.clear();

    foreach (KJob* job, m_previewJobs) {
        job->kill();
    }
    Q_ASSERT(m_previewJobs.isEmpty());
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

    resolvePendingRoles();
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

    resolvePendingRoles();
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
            data.insert("iconPixmap", QPixmap());
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

    const bool getSizeRole = m_roles.contains("size");
    const bool getIsExpandableRole = m_roles.contains("isExpandable");

    if ((getSizeRole || getIsExpandableRole) && item.isDir()) {
        if (item.isLocalFile()) {
            const QString path = item.localPath();
            const int count = subItemsCount(path);
            if (getSizeRole) {
                data.insert("size", count);
            }
            if (getIsExpandableRole) {
                data.insert("isExpandable", count > 0);
            }
        } else if (getSizeRole) {
            data.insert("size", -1); // -1 indicates an unknown number of items
        }
    }

    if (m_roles.contains("type")) {
        data.insert("type", item.mimeComment());
    }

    data.insert("iconOverlays", item.overlays());

#ifdef HAVE_NEPOMUK
    if (m_nepomukResourceWatcher) {
        const KNepomukRolesProvider& rolesProvider = KNepomukRolesProvider::instance();
        Nepomuk::Resource resource(item.url());
        QHashIterator<QByteArray, QVariant> it(rolesProvider.roleValues(resource, m_roles));
        while (it.hasNext()) {
            it.next();
            data.insert(it.key(), it.value());
        }

        QUrl uri = resource.resourceUri();
        if (uri.isEmpty()) {
            // TODO: Is there another way to explicitly create a resource?
            // We need a resource to be able to track it for changes.
            resource.setRating(0);
            uri = resource.resourceUri();
        }
        if (!uri.isEmpty() && !m_nepomukUriItems.contains(uri)) {
            // TODO: Calling stop()/start() is a workaround until
            // ResourceWatcher has been fixed.
            m_nepomukResourceWatcher->stop();
            m_nepomukResourceWatcher->addResource(resource);
            m_nepomukResourceWatcher->start();

            m_nepomukUriItems.insert(uri, item.url());
        }
    }
#endif

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

int KFileItemModelRolesUpdater::subItemsCount(const QString& path) const
{
    const bool countHiddenFiles = m_model->showHiddenFiles();
    const bool showFoldersOnly  = m_model->showFoldersOnly();

#ifdef Q_WS_WIN
    QDir dir(path);
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::System;
    if (countHiddenFiles) {
        filters |= QDir::Hidden;
    }
    if (showFoldersOnly) {
        filters |= QDir::Dirs;
    } else {
        filters |= QDir::AllEntries;
    }
    return dir.entryList(filters).count();
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
                if (dirEntry->d_name[1] == '\0' || !countHiddenFiles) {
                    // Skip "." or hidden files
                    continue;
                }
                if (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0') {
                    // Skip ".."
                    continue;
                }
            }

            // If only directories are counted, consider an unknown file type also
            // as directory instead of trying to do an expensive stat() (see bug 292642).
            if (!showFoldersOnly || dirEntry->d_type == DT_DIR || dirEntry->d_type == DT_UNKNOWN) {
                ++count;
            }
        }
        ::closedir(dir);
    }
    return count;
#endif
}

void KFileItemModelRolesUpdater::updateAllPreviews()
{
    if (m_paused) {
        m_previewChangedDuringPausing = true;
    } else {
        sortAndResolveAllRoles();
    }
}

#include "kfileitemmodelrolesupdater.moc"
