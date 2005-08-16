/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __bookmarkmodel_h
#define __bookmarkmodel_h

#include <q3valuevector.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <kbookmark.h>
#include "toplevel.h"
#include "treeitem.h"
#include "commands.h"

#include <kdebug.h>

#include <kurl.h>

class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

protected:
    class insertSentry;
    friend class insertSentry;
    class removeSentry;
    friend class removeSentry;
    friend class IKEBCommand;

public:    
    static BookmarkModel* self() { if(!s_bookmarkModel) s_bookmarkModel = new BookmarkModel(CurrentMgr::self()->root()); return s_bookmarkModel; }
    virtual ~BookmarkModel() {}

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QModelIndex bookmarkToIndex(KBookmark bk);
    static int BookmarkModel::childCount(KBookmark bk);
    void emitDataChanged(KBookmark bk);

    void resetModel();

private:
    TreeItem * rootItem;
    BookmarkModel(KBookmark root)
        :QAbstractItemModel(), mRoot(root)
        { rootItem = new TreeItem(root, 0); }
    static BookmarkModel *s_bookmarkModel;
    static int count;
    KBookmark mRoot;
    // mutable QList<KBookmark> mapIdToAddr;

//Sentry
protected:
    class insertSentry
    {
        public:
        insertSentry(KBookmark parent, int first, int last)
            {
                QModelIndex mParent = BookmarkModel::self()->bookmarkToIndex(parent);
                BookmarkModel::self()->beginInsertRows( mParent, first, last);

                mt = static_cast<TreeItem *>(mParent.internalPointer());
                mf = first;
                ml = last;
            }
        ~insertSentry()
            {
                mt->insertChildren(mf, ml);
                BookmarkModel::self()->endInsertRows(); 
            }
        private:
            TreeItem * mt;
            int mf, ml;
    };
    class removeSentry
    {
        public:
        removeSentry(KBookmark parent, int first, int last)
            { 
                QModelIndex mParent = BookmarkModel::self()->bookmarkToIndex(parent);
                //FIXME remove this once Qt fixes their really stupid bugs
                for(int i = first; i <= last; ++i)
                {
                    KEBApp::self()->mBookmarkListView->selectionModel()->select(mParent.child(i, 0), QItemSelectionModel::Deselect);
                }

                BookmarkModel::self()->beginRemoveRows( mParent, first, last); 

                mt = static_cast<TreeItem *>(mParent.internalPointer());
                mf = first;
                ml = last;
            }
        ~removeSentry()
            {
               mt->deleteChildren(mf, ml);
               BookmarkModel::self()->endRemoveRows(); 
            }
        private:
            TreeItem * mt;
            int mf, ml;
    };
};

#endif
