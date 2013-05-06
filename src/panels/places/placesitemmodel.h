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

#ifndef PLACESITEMMODEL_H
#define PLACESITEMMODEL_H

#include <config-nepomuk.h>

#include <kitemviews/kstandarditemmodel.h>

#include <KUrl>
#include <QHash>
#include <QList>
#include <QSet>
#include <Solid/Predicate>
#include <Solid/StorageAccess>

class KBookmark;
class KBookmarkManager;
class PlacesItem;
class QAction;
class QTimer;

#ifdef HAVE_NEPOMUK
    namespace Nepomuk2
    {
        namespace Query
        {
            class Term;
        }
    }
#endif

// #define PLACESITEMMODEL_DEBUG

/**
 * @brief Model for maintaining the bookmarks of the places panel.
 *
 * It is compatible to the KFilePlacesModel from kdelibs but adds
 * the ability to have groups for places.
 */
class PlacesItemModel: public KStandardItemModel
{
    Q_OBJECT

public:
    explicit PlacesItemModel(QObject* parent = 0);
    virtual ~PlacesItemModel();

    /**
     * @return A new instance of a places item with the given
     *         attributes.
     */
    PlacesItem* createPlacesItem(const QString& text,
                                 const KUrl& url,
                                 const QString& iconName = QString());

    PlacesItem* placesItem(int index) const;

    /**
     * If set to true, all items that are marked as hidden
     * will be shown in the view. The items will
     * stay marked as hidden, which is visually indicated
     * by the view by desaturating the icon and the text.
     */
    void setHiddenItemsShown(bool show);
    bool hiddenItemsShown() const;

    /**
     * @return Number of items that are marked as hidden.
     *         Note that this does not mean that the items
     *         are really hidden
     *         (see PlacesItemModel::setHiddenItemsShown()).
     */
    int hiddenCount() const;

    /**
     * Search the item which is equal to the URL or at least
     * is a parent URL. If there are more than one possible
     * candidates, return the item which covers the biggest
     * range of the URL. -1 is returned if no closest item
     * could be found.
     */
    int closestItem(const KUrl& url) const;

    /**
     * Appends the item \a item as last element of the group
     * the item belongs to. If no item with the same group is
     * present, the item gets appended as last element of the
     * model. PlacesItemModel takes the ownership
     * of the item.
     */
    void appendItemToGroup(PlacesItem* item);

    QAction* ejectAction(int index) const;
    QAction* teardownAction(int index) const;

    void requestEject(int index);
    void requestTeardown(int index);

    bool storageSetupNeeded(int index) const;
    void requestStorageSetup(int index);

    /** @reimp */
    virtual QMimeData* createMimeData(const QSet<int>& indexes) const;

    /** @reimp */
    virtual bool supportsDropping(int index) const;

    void dropMimeDataBefore(int index, const QMimeData* mimeData);

    /**
     * @return Converts the URL, which contains "virtual" URLs for system-items like
     *         "search:/documents" into a Nepomuk-Query-URL that will be handled by
     *         the corresponding IO-slave. Virtual URLs for bookmarks are used to
     *         be independent from internal format changes.
     */
    static KUrl convertedUrl(const KUrl& url);

    virtual void clear();
signals:
    void errorMessage(const QString& message);
    void storageSetupDone(int index, bool success);

protected:
    virtual void onItemInserted(int index);
    virtual void onItemRemoved(int index, KStandardItem* removedItem);
    virtual void onItemChanged(int index, const QSet<QByteArray>& changedRoles);

private slots:
    void slotDeviceAdded(const QString& udi);
    void slotDeviceRemoved(const QString& udi);
    void slotStorageTeardownDone(Solid::ErrorType error, const QVariant& errorData);
    void slotStorageSetupDone(Solid::ErrorType error, const QVariant& errorData, const QString& udi);
    void hideItem();

    /**
     * Updates the bookmarks from the model corresponding to the changed
     * bookmarks stored by the bookmark-manager. Is called whenever the bookmarks
     * have been changed by another application.
     */
    void updateBookmarks();

    /**
     * Saves the bookmarks and indicates to other applications that the
     * state of the bookmarks has been changed. Is only called by the
     * timeout of m_saveBookmarksTimer to prevent unnecessary savings.
     */
    void saveBookmarks();

    void slotNepomukStarted();
    void slotNepomukStopped();
private:
    struct SystemBookmarkData;

    /**
     * Loads the bookmarks from the bookmark-manager and creates items for
     * the model or moves hidden items to m_bookmarkedItems.
     */
    void loadBookmarks();

    /**
     * @return True, if the bookmark can be accepted in the context of the
     *         current application (e.g. bookmarks from other applications
     *         will be ignored).
     */
    bool acceptBookmark(const KBookmark& bookmark,
                        const QSet<QString>& availableDevices) const;

    /**
     * Creates a PlacesItem for a system-bookmark:
     * - PlacesItem::isSystemItem() will return true
     * - Default view-properties will be created for "Search For" items
     * The item is not inserted to the model yet.
     */
    PlacesItem* createSystemPlacesItem(const SystemBookmarkData& data);

    /**
     * Creates system bookmarks that are shown per default and can
     * only be hidden but not removed. The result will be stored
     * in m_systemBookmarks.
     */
    void createSystemBookmarks();

    void initializeAvailableDevices();

    /**
     * @param index Item index related to the model.
     * @return      Corresponding index related to m_bookmarkedItems.
     */
    int bookmarkIndex(int index) const;

    /**
     * Marks the item with the index \a index as hidden and
     * removes it from the model so that it gets invisible.
     */
    void hideItem(int index);

    /**
     * Triggers a delayed saving of bookmarks by starting
     * m_saveBookmarksTimer.
     */
    void triggerBookmarksSaving();

    QString internalMimeType() const;

    /**
     * @return Adjusted drop index which assures that the item is aligned
     *         into the same group as specified by PlacesItem::groupType().
     */
    int groupedDropIndex(int index, const PlacesItem* item) const;

    /**
     * @return True if the bookmarks have the same identifiers. The identifier
     *         is the unique "ID"-property in case if no UDI is set, otherwise
     *         the UDI is used as identifier.
     */
    static bool equalBookmarkIdentifiers(const KBookmark& b1, const KBookmark& b2);

    /**
     * @return URL using the timeline-protocol for searching (see convertedUrl()).
     */
    static KUrl createTimelineUrl(const KUrl& url);

    /**
     * Helper method for createTimelineUrl().
     * @return String that represents a date-path in the format that
     *         the timeline-protocol expects.
     */
    static QString timelineDateString(int year, int month, int day = 0);

    /**
     * @return URL that can be listed by KIO and results in searching
     *         for a given term. The URL \a url represents a places-internal
     *         URL like e.g. "search:/documents" (see convertedUrl()).
     */
    static KUrl createSearchUrl(const KUrl& url);

#ifdef HAVE_NEPOMUK
    /**
     * Helper method for createSearchUrl().
     * @return URL that can be listed by KIO and results in searching
     *         for the given term.
     */
    static KUrl searchUrlForTerm(const Nepomuk2::Query::Term& term);
#endif

#ifdef PLACESITEMMODEL_DEBUG
    void showModelState();
#endif

private:
    bool m_fileIndexingEnabled;
    bool m_hiddenItemsShown;

    QSet<QString> m_availableDevices;
    Solid::Predicate m_predicate;
    KBookmarkManager* m_bookmarkManager;

    struct SystemBookmarkData
    {
        SystemBookmarkData(const KUrl& url,
                           const QString& icon,
                           const QString& text) :
            url(url), icon(icon), text(text) {}
        KUrl url;
        QString icon;
        QString text;
    };

    QList<SystemBookmarkData> m_systemBookmarks;
    QHash<KUrl, int> m_systemBookmarksIndexes;

    // Contains hidden and unhidden items that are stored as
    // bookmark (the model itself only contains items that
    // are shown in the view). If an entry is 0, then the
    // places-item is part of the model. If an entry is not
    // 0, the item is hidden and not part of the model.
    QList<PlacesItem*> m_bookmarkedItems;

    // Index of the hidden item that should be removed in
    // removeHiddenItem(). The removing must be done
    // asynchronously as in the scope of onItemChanged()
    // removing an item is not allowed.
    int m_hiddenItemToRemove;

    QTimer* m_saveBookmarksTimer;
    QTimer* m_updateBookmarksTimer;

    QHash<QObject*, int> m_storageSetupInProgress;
};

#endif


