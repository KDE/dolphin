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

#ifndef KFILEITEMMODELROLESUPDATER_H
#define KFILEITEMMODELROLESUPDATER_H

#include <config-nepomuk.h>

#include <KFileItem>
#include <kitemviews/kitemmodelbase.h>

#include <libdolphin_export.h>

#include <QObject>
#include <QSet>
#include <QSize>
#include <QStringList>

class KFileItemModel;
class KJob;
class QPixmap;
class QTimer;

#ifdef HAVE_NEPOMUK
    namespace Nepomuk
    {
        class ResourceWatcher;
        class Resource;
    }
#else
    // Required for the slot applyChangedNepomukRoles() that
    // cannot be ifdefined due to moc.
    namespace Nepomuk
    {
        class Resource;
    }
#endif

/**
 * @brief Resolves expensive roles asynchronously and applies them to the KFileItemModel.
 *
 * KFileItemModel only resolves roles that are inexpensive like e.g. the file name or
 * the permissions. Creating previews or determining the MIME-type can be quite expensive
 * and KFileItemModelRolesUpdater takes care to update such roles asynchronously.
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModelRolesUpdater : public QObject
{
    Q_OBJECT

public:
    explicit KFileItemModelRolesUpdater(KFileItemModel* model, QObject* parent = 0);
    virtual ~KFileItemModelRolesUpdater();

    void setIconSize(const QSize& size);
    QSize iconSize() const;

    /**
     * Sets the range of items that are visible currently. The roles
     * of visible items are resolved first.
     */
    void setVisibleIndexRange(int index, int count);

    /**
     * If \a show is set to true, the "iconPixmap" role will be filled with a preview
     * of the file. If \a show is false the MIME type icon will be used for the "iconPixmap"
     * role.
     */
    void setPreviewShown(bool show);
    bool isPreviewShown() const;

    /**
     * If \a paused is set to true the asynchronous resolving of roles will be paused.
     * State changes during pauses like changing the icon size or the preview-shown
     * will be remembered and handled after unpausing.
     */
    void setPaused(bool paused);
    bool isPaused() const;

    /**
     * Sets the roles that should be resolved asynchronously.
     */
    void setRoles(const QSet<QByteArray>& roles);
    QSet<QByteArray> roles() const;

    /**
     * Sets the list of enabled thumbnail plugins that are used for previews.
     * Per default all plugins enabled in the KConfigGroup "PreviewSettings"
     * are used.
     *
     * For a list of available plugins, call KServiceTypeTrader::self()->query("ThumbCreator").
     *
     * @see enabledPlugins
     */
    void setEnabledPlugins(const QStringList& list);

    /**
     * Returns the list of enabled thumbnail plugins.
     * @see setEnabledPlugins
     */
    QStringList enabledPlugins() const;

private slots:
    void slotItemsInserted(const KItemRangeList& itemRanges);
    void slotItemsRemoved(const KItemRangeList& itemRanges);
    void slotItemsChanged(const KItemRangeList& itemRanges,
                          const QSet<QByteArray>& roles);

    /**
     * Is invoked after a preview has been received successfully.
     * @see startPreviewJob()
     */
    void slotGotPreview(const KFileItem& item, const QPixmap& pixmap);

    /**
     * Is invoked after generating a preview has failed.
     * @see startPreviewJob()
     */
    void slotPreviewFailed(const KFileItem& item);

    /**
     * Is invoked when the preview job has been finished and
     * removes the job from the m_previewJobs list.
     * @see startPreviewJob()
     */
    void slotPreviewJobFinished(KJob* job);

    void resolveNextPendingRoles();

    /**
     * Resolves items that have not been resolved yet after the change has been
     * notified by slotItemsChanged(). Is invoked if the m_changedItemsTimer
     * exceeds.
     */
    void resolveChangedItems();

    void applyChangedNepomukRoles(const Nepomuk::Resource& resource);

private:
    /**
     * Updates the roles for the given item ranges. The roles for the currently
     * visible items will get updated first.
     */
    void startUpdating(const KItemRangeList& itemRanges);

    /**
     * Creates previews for the items starting from the first item of the
     * given list.
     * @see slotGotPreview()
     * @see slotPreviewFailed()
     * @see slotPreviewJobFinished()
     */
    void startPreviewJob(const KFileItemList& items);

    bool hasPendingRoles() const;
    void resolvePendingRoles();
    void resetPendingRoles();
    void sortAndResolveAllRoles();
    void sortAndResolvePendingRoles();

    enum ResolveHint {
        ResolveFast,
        ResolveAll
    };
    bool applyResolvedRoles(const KFileItem& item, ResolveHint hint);
    QHash<QByteArray, QVariant> rolesData(const KFileItem& item) const;

    KFileItemList sortedItems(const QSet<KFileItem>& items) const;

    /**
     * @return The number of items of the path \a path.
     */
    int subItemsCount(const QString& path) const;

private:
    // Property for setPaused()/isPaused().
    bool m_paused;

    // Property changes during pausing must be remembered to be able
    // to react when unpausing again:
    bool m_previewChangedDuringPausing;
    bool m_iconSizeChangedDuringPausing;
    bool m_rolesChangedDuringPausing;

    // Property for setPreviewShown()/previewShown().
    bool m_previewShown;

    // True if the role "iconPixmap" should be cleared when resolving the next
    // role with resolveRole(). Is necessary if the preview gets disabled
    // during the roles-updater has been paused by setPaused().
    bool m_clearPreviews;

    KFileItemModel* m_model;
    QSize m_iconSize;
    int m_firstVisibleIndex;
    int m_lastVisibleIndex;
    QSet<QByteArray> m_roles;
    QStringList m_enabledPlugins;

    QSet<KFileItem> m_pendingVisibleItems;
    QSet<KFileItem> m_pendingInvisibleItems;
    QList<KJob*> m_previewJobs;

    // When downloading or copying large files, the slot slotItemsChanged()
    // will be called periodically within a quite short delay. To prevent
    // a high CPU-load by generating e.g. previews for each notification, the update
    // will be postponed until no file change has been done within a longer period
    // of time.
    QTimer* m_changedItemsTimer;
    QSet<KFileItem> m_changedItems;

#ifdef HAVE_NEPOMUK
    Nepomuk::ResourceWatcher* m_nepomukResourceWatcher;
    mutable QHash<QUrl, KUrl> m_nepomukUriItems;
#endif
};

#endif


