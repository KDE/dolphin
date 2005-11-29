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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __bookmarkmodel_h
#define __bookmarkmodel_h

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QVector>
#include <kbookmark.h>
#include "toplevel.h"
#include "treeitem.h"

class QDropEvent;

class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    class insertSentry;
    friend class insertSentry;
    class removeSentry;
    friend class removeSentry;

    static BookmarkModel* self();
    virtual ~BookmarkModel();

    //reimplemented functions
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual void resetModel();

    QModelIndex bookmarkToIndex(KBookmark bk);
    void emitDataChanged(KBookmark bk);

    //drag and drop
    virtual bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QStringList mimeTypes () const;
    virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
    virtual Qt::DropActions supportedDropActions () const;

    // Called by BookmarkListView::dropEvent to save the pointer
    // The pointer to the event is retrieved in dropMimeData()
    void saveDropEventPointer(QDropEvent * event);

signals:
    //FIXME searchline should respond too
    void aboutToMoveRows(const QModelIndex &, int, int, const QModelIndex &, int);
    void rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);

private:
    void beginMoveRows(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position);
    void endMoveRows();

    TreeItem * rootItem;
    BookmarkModel(KBookmark root);
    static BookmarkModel *s_bookmarkModel;
    static int count;
    KBookmark mRoot;

    // for move support
    QModelIndex mOldParent;
    int mFirst;
    int mLast;
    QModelIndex mNewParent;
    int mPosition;
    QVector<QModelIndex> movedIndexes;
    QVector<QModelIndex> oldParentIndexes;
    QVector<QModelIndex> newParentIndexes;

    QDropEvent * dropEvent;

//Sentry
public:
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

    class moveSentry
    {
        public:
        moveSentry(KBookmark oldParent, int first, int last, KBookmark newParent, int position)
            {
                //FIXME need to decide how to handle selections and moving.
                KEBApp::self()->mBookmarkListView->selectionModel()->clear();
                QModelIndex mOldParent = BookmarkModel::self()->bookmarkToIndex(oldParent);
                QModelIndex mNewParent = BookmarkModel::self()->bookmarkToIndex(newParent);

                BookmarkModel::self()->beginMoveRows( mOldParent, first, last, mNewParent, position); 

                mop = static_cast<TreeItem *>(mOldParent.internalPointer());
                mf = first;
                ml = last;
                mnp = static_cast<TreeItem *>(mNewParent.internalPointer());
                mp = position;
            }
        ~moveSentry()
            {
               mop->moveChildren(mf, ml, mnp, mp);
               BookmarkModel::self()->endMoveRows(); 
            }
        private:
            TreeItem * mop, *mnp;
            int mf, ml, mp;
    };

};

#endif
