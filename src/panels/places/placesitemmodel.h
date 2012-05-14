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

class KBookmarkManager;
class PlacesItem;
class QAction;
class QTimer;

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

    PlacesItem* createPlacesItem(const QString& text,
                                 const KUrl& url,
                                 const QString& iconName);

    PlacesItem* placesItem(int index) const;

    void setHiddenItemsShown(bool show);
    bool hiddenItemsShown() const;

    int hiddenCount() const;

    /**
     * Search the item which is equal to the URL or at least
     * is a parent URL. If there are more than one possible
     * candidates, return the item which covers the biggest
     * range of the URL. -1 is returned if no closest item
     * could be found.
     */
    int closestItem(const KUrl& url) const;

    QAction* ejectAction(int index) const;
    QAction* teardownAction(int index) const;

    void requestEject(int index);
    void requestTeardown(int index);

signals:
    void errorMessage(const QString& message);

protected:
    virtual void onItemInserted(int index);
    virtual void onItemRemoved(int index, KStandardItem* removedItem);
    virtual void onItemChanged(int index, const QSet<QByteArray>& changedRoles);

private slots:
    void slotDeviceAdded(const QString& udi);
    void slotDeviceRemoved(const QString& udi);
    void slotStorageTeardownDone(Solid::ErrorType error, const QVariant& errorData);
    void removeHiddenItem();
    void saveBookmarks();

private:
    void loadBookmarks();   

    /**
     * Helper method for loadBookmarks(): Adds the items
     * to the model if the "isHidden"-property is false,
     * otherwise the items get added to m_hiddenItems.
     */
    void addItems(const QList<PlacesItem*>& items);

    /**
     * Creates system bookmarks that are shown per default and can
     * only be hidden but not removed. The result will be stored
     * in m_systemBookmarks.
     */
    void createSystemBookmarks();

    void initializeAvailableDevices();

    /**
     * @param index Item index related to the model.
     * @return      Corresponding item index related to m_hiddenItems.
     */
    int hiddenIndex(int index) const;

    void removeHiddenItem(int index);

#ifdef PLACESITEMMODEL_DEBUG
    void showModelState();
#endif

private:
    bool m_nepomukRunning;
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

    QList<PlacesItem*> m_hiddenItems;

    // Index of the hidden item that should be removed in
    // removeHiddenItem(). The removing must be done
    // asynchronously as in the scope of onItemChanged()
    // removing an item is not allowed.
    int m_hiddenItemToRemove;

    QTimer* m_saveBookmarksTimer;
};

#endif


