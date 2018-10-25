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

#include "kitemviews/kstandarditemmodel.h"

#include <KFilePlacesModel>
#include <Solid/Predicate>
#include <Solid/StorageAccess>

#include <QHash>
#include <QList>
#include <QSet>
#include <QUrl>

class KBookmark;
class PlacesItem;
class QAction;

/**
 * @brief Model for maintaining the bookmarks of the places panel.
 *
 * It is based on KFilePlacesModel from KIO.
 */
class PlacesItemModel: public KStandardItemModel
{
    Q_OBJECT

public:
    explicit PlacesItemModel(QObject* parent = nullptr);
    ~PlacesItemModel() override;

    /**
     * @brief Create a new place entry in the bookmark file
     * and add it to the model
     */
    void createPlacesItem(const QString& text,
                          const QUrl& url,
                          const QString& iconName = QString(),
                          int after = -1);

    PlacesItem* placesItem(int index) const;

    /**
     * @brief Mark an item as hidden
     * @param index of the item to be hidden
     */
    void hideItem(int index);

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
    int closestItem(const QUrl& url) const;

    QAction* ejectAction(int index) const;
    QAction* teardownAction(int index) const;

    void requestEject(int index);
    void requestTearDown(int index);

    bool storageSetupNeeded(int index) const;
    void requestStorageSetup(int index);

    QMimeData* createMimeData(const KItemSet& indexes) const override;

    bool supportsDropping(int index) const override;

    void dropMimeDataBefore(int index, const QMimeData* mimeData);

    /**
     * @return Converts the URL, which contains "virtual" URLs for system-items like
     *         "search:/documents" into a Query-URL that will be handled by
     *         the corresponding IO-slave. Virtual URLs for bookmarks are used to
     *         be independent from internal format changes.
     */
    static QUrl convertedUrl(const QUrl& url);

    void clear() override;

    void proceedWithTearDown();

    /**
     * @brief Remove item from bookmark
     *
     * This function remove the index from bookmark file permanently
     *
     * @param index - the item to be removed
     */
    void deleteItem(int index);

    /**
    * Force a sync on the bookmarks and indicates to other applications that the
    * state of the bookmarks has been changed.
    */
    void refresh();

    bool isDir(int index) const override;


    KFilePlacesModel::GroupType groupType(int row) const;
    bool isGroupHidden(KFilePlacesModel::GroupType type) const;
    void setGroupHidden(KFilePlacesModel::GroupType type, bool hidden);

signals:
    void errorMessage(const QString& message);
    void storageSetupDone(int index, bool success);
    void storageTearDownRequested(const QString& mountPath);
    void storageTearDownExternallyRequested(const QString& mountPath);

protected:
    void onItemInserted(int index) override;
    void onItemRemoved(int index, KStandardItem* removedItem) override;
    void onItemChanged(int index, const QSet<QByteArray>& changedRoles) override;

private slots:
    void slotStorageTearDownDone(Solid::ErrorType error, const QVariant& errorData);
    void slotStorageSetupDone(Solid::ErrorType error, const QVariant& errorData, const QString& udi);

    // source model control
    void onSourceModelRowsInserted(const QModelIndex &parent, int first, int last);
    void onSourceModelRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onSourceModelRowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onSourceModelRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onSourceModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onSourceModelGroupHiddenChanged(KFilePlacesModel::GroupType group, bool hidden);

private:
    /**
     * Remove bookmarks created by the previous version of dolphin that are
     * not valid anymore
     */
    void cleanupBookmarks();

    /**
     * Loads the bookmarks from the bookmark-manager and creates items for
     * the model or moves hidden items to m_bookmarkedItems.
     */
    void loadBookmarks();

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
     * Appends the item \a item as last element of the group
     * the item belongs to. If no item with the same group is
     * present, the item gets appended as last element of the
     * model. PlacesItemModel takes the ownership
     * of the item.
     */
    void insertSortedItem(PlacesItem* item);

    PlacesItem *itemFromBookmark(const KBookmark &bookmark) const;

    void addItemFromSourceModel(const QModelIndex &index);
    void removeItemByIndex(const QModelIndex &mapToSource);

    QString bookmarkId(const KBookmark &bookmark) const;
    void initializeDefaultViewProperties() const;

    int mapFromSource(const QModelIndex &index) const;
    QModelIndex mapToSource(int row) const;

    static void updateItem(PlacesItem *item, const QModelIndex &index);

private:
    bool m_hiddenItemsShown;

    Solid::StorageAccess *m_deviceToTearDown;

    QHash<QObject*, int> m_storageSetupInProgress;

    KFilePlacesModel *m_sourceModel;

    QVector<QPersistentModelIndex> m_indexMap;
};

#endif


