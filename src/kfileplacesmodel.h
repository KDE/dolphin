/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>
    Copyright (C) 2007 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef KFILEPLACESMODEL_H
#define KFILEPLACESMODEL_H


#include <QAbstractItemModel>
#include <kurl.h>
#include <kicon.h>

/**
 * This class is a list view model. Each entry represents a "place"
 * where user can access files. Only revelant when
 * used with QListView or QTableView.
 */
class /*KIO_EXPORT*/ KFilePlacesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum AdditionalRoles {
        UrlRole = 0x069CD12B,
        HiddenRole = 0x0741CAAC,
        MountNeededRole = 0x059A935D
    };

    KFilePlacesModel(QObject *parent=0);
    ~KFilePlacesModel();

    KUrl url(const QModelIndex &index) const;
    bool mountNeeded(const QModelIndex &index) const;
    KIcon icon(const QModelIndex &index) const;
    QString text(const QModelIndex &index) const;

    /**
     * @brief Get a visible data based on Qt role for the given index.
     * Return the device information for the give index.
     *
     * @param index The QModelIndex which contains the row, column to fetch the data.
     * @param role The Interview data role(ex: Qt::DisplayRole).
     *
     * @return the data for the given index and role.
     */
    QVariant data(const QModelIndex &index, int role) const;

    /**
     * @brief Get the children model index for the given row and column.
     */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    /**
     * @brief Get the parent QModelIndex for the given model child.
     */
    QModelIndex parent(const QModelIndex &child) const;

    /**
     * @brief Get the number of rows for a model index.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /**
     * @brief Get the number of columns for a model index.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /**
     * Returns the closest item for the URL \a url.
     * The closest item is defined as item which is equal to
     * the URL or at least is a parent URL. If there are more than
     * one possible parent URL candidates, the item which covers
     * the bigger range of the URL is returned.
     *
     * Example: the url is '/home/peter/Documents/Music'.
     * Available items are:
     * - /home/peter
     * - /home/peter/Documents
     *
     * The returned item will the one for '/home/peter/Documents'.
     */
    QModelIndex closestItem(const KUrl &url) const;

private:
    Q_PRIVATE_SLOT(d, void _k_devicesInserted(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d, void _k_devicesRemoved(const QModelIndex&, int, int))
    Q_PRIVATE_SLOT(d, void _k_reloadBookmarks())

    class Private;
    Private * const d;
    friend class Private;
};

#endif
