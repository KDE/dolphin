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

#include "bookmarkmodel.h"
#include "toplevel.h"
#include "commands.h"

#include <kcommand.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <klocale.h>
#include <QIcon>
#include <QPixmap>
#include <QStringList>
#include <QMimeData>
#include <kdebug.h>

int BookmarkModel::count = 0;
BookmarkModel* BookmarkModel::s_bookmarkModel = 0L;

BookmarkModel* BookmarkModel::self()
{
    if(!s_bookmarkModel)     
        s_bookmarkModel = new BookmarkModel(CurrentMgr::self()->root());    
    return s_bookmarkModel;
}

BookmarkModel::BookmarkModel(KBookmark root)
    :QAbstractItemModel(), mRoot(root)
{
    rootItem = new TreeItem(root, 0);
}

BookmarkModel::~BookmarkModel()
{
    delete rootItem;
}

void BookmarkModel::resetModel()
{
    delete rootItem;
    reset();
    rootItem = new TreeItem(mRoot, 0);
}

void BookmarkModel::beginMoveRows(const QModelIndex & oldParent, int first, int last, const QModelIndex & newParent, int position)
{
    emit aboutToMoveRows(oldParent, first, last, newParent, position);
    if(oldParent != newParent) // different parents
    {
        int columnsCount = columnCount(QModelIndex());
        for(int i=first; i<=last; ++i)
            for(int j=0; j<columnsCount; ++j)
                movedIndexes.push_back( index(i, j, oldParent));

        int rowsCount = rowCount(oldParent);
        for(int i=last+1; i<rowsCount; ++i)
            for(int j=0; j<columnsCount; ++j)
                oldParentIndexes.push_back( index(i, j, oldParent));

        rowsCount = rowCount(newParent);
        for(int i=position; i<rowsCount; ++i)
            for(int j=0; j<columnsCount; ++j)
                newParentIndexes.push_back( index(i, j, newParent));
    }
    else //same parent
    {
        int columnsCount = columnCount(QModelIndex());
        kdDebug()<<" before "<<first<<" "<<last<<" "<<position<<endl;
        if(first > position)
        {
            // swap around 
            int tempPos = position;
            position = last + 1;
            last = first - 1;
            first = tempPos;
        }
        kdDebug()<<" after "<<first<<" "<<last<<" "<<position<<endl;

        kdDebug()<<" strange things ? "<<static_cast<TreeItem *>(oldParent.internalPointer())<<endl;
        // Invariant first > positionx
        for(int i=first; i<=last; ++i)
            for(int j=0; j<columnsCount; ++j)
                movedIndexes.push_back( index(i, j, oldParent));

        for(int i=last+1; i<position; ++i)
            for(int j=0; j<columnsCount; ++j)
                oldParentIndexes.push_back( index(i, j, oldParent));
    }
    mOldParent = oldParent;
    mFirst = first;
    mLast = last;
    mNewParent = newParent;
    mPosition = position;
}
void BookmarkModel::endMoveRows()
{
    if(mOldParent != mNewParent)
    {
        int count = movedIndexes.count();
        int delta = mPosition - mFirst;
        for(int i=0; i <count; ++i)
        {
            QModelIndex idx = createIndex(movedIndexes[i].row()+delta, 
                                          movedIndexes[i].column(), 
                                          movedIndexes[i].internalPointer());
            changePersistentIndex( movedIndexes[i], idx);
        }

        count = oldParentIndexes.count();
        delta = mLast - mFirst + 1;
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex( oldParentIndexes[i].row()-delta, 
                                           oldParentIndexes[i].column(), 
                                           oldParentIndexes[i].internalPointer());
            changePersistentIndex( oldParentIndexes[i], idx);
        }

        count = newParentIndexes.count();
        for(int i=0; i<count; ++i)
        {
            int delta = (mLast-mFirst+1);
            QModelIndex idx = createIndex( newParentIndexes[i].row() + delta,
                                           newParentIndexes[i].column(), 
                                           newParentIndexes[i].internalPointer());
            changePersistentIndex( newParentIndexes[i], idx);
        }
    }
    else
    {
        int count = movedIndexes.count();
        int delta = mPosition - (mLast +1);
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex(movedIndexes[i].row()+delta, movedIndexes[i].column(), movedIndexes[i].internalPointer());
            changePersistentIndex(movedIndexes[i], idx);
        }
        
        count = oldParentIndexes.count();
        delta = mLast-mFirst + 1;
        for(int i=0; i<count; ++i)
        {
            QModelIndex idx = createIndex(oldParentIndexes[i].row() - delta, oldParentIndexes[i].column(), oldParentIndexes[i].internalPointer());
            changePersistentIndex(oldParentIndexes[i], idx);
        }
    }
    emit rowsMoved(mOldParent, mFirst, mLast, mNewParent, mPosition);
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    //Text
    if(index.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
    {
        KBookmark bk = static_cast<TreeItem *>(index.internalPointer())->bookmark();
        if(bk.address() == "")
            if(index.column() == 0)
                return QVariant( i18n("Bookmarks") );
            else
                return QVariant();

        switch( index.column() )
        {
            case 0:
                return QVariant( bk.fullText() );
            case 1:
                return QVariant( bk.url().pathOrURL() ); 
            case 2:
                return QVariant( EditCommand::getNodeText(bk, QStringList() << QString("desc")) );
            case 3:
                return QVariant( QString() ); //FIXME status column
            default:
                return QVariant( QString() ); //can't happen
        }
    }

    //Icon
    if(index.isValid() && role == Qt::DecorationRole && index.column() == 0)
    {
        KBookmark bk = static_cast<TreeItem *>(index.internalPointer())->bookmark();
        if(bk.address() == "")
            return QVariant( QIcon(SmallIcon("bookmark")));
        return QVariant( QIcon(SmallIcon(bk.icon())));
    }
    return QVariant();
}

//FIXME QModelIndex BookmarkModel::buddy(const QModelIndex & index) //return parent for empty folder padders


Qt::ItemFlags BookmarkModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    KBookmark bk = static_cast<TreeItem *>(index.internalPointer())->bookmark();
    if( bk.address() != ""  ) // non root
    {
        if( bk.isGroup())
        {
            if(index.column() == 0 || index.column() == 2)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
        else
            if(index.column() < 3)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    // root
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool BookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid() && role == Qt::EditRole)
    {
        //FIXME don't create a commmand if still the same
        // and ignore if name column is empty
        QString addr = static_cast<TreeItem *>(index.internalPointer())->bookmark().address();
        CmdHistory::self()->addCommand(new EditCommand( addr, index.column(), value.toString()) );
        return true;
    }
    return false;
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch(section)
        {
            case 0:
                return i18n("Bookmark");
            case 1:
                return i18n("URL");
            case 2:
                return i18n("Comment");
            case 3:
                return i18n("Status");
            default:
                return QString(); // Can't happpen
        }
    }
    else
        return QVariant();
}

QModelIndex BookmarkModel::index(int row, int column, const QModelIndex &parent) const
{
    if( ! parent.isValid())
        return createIndex(row, column, rootItem);

    TreeItem * item = static_cast<TreeItem *>(parent.internalPointer());
    return createIndex(row, column, item->child(row));
}



QModelIndex BookmarkModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
    {
        // qt asks for the parent of an invalid parent
        // either we are in a inconsistent case or more likely
        // we are dragging and dropping and qt didn't check
        // what itemAt() returned
        return index;

    }
    KBookmark bk = static_cast<TreeItem *>(index.internalPointer())->bookmark();

    if(bk.address() == mRoot.address())
        return QModelIndex();

    KBookmarkGroup parent  = bk.parentGroup();
    TreeItem * item = static_cast<TreeItem *>(index.internalPointer());
    if(parent.address() != mRoot.address())
        return createIndex( KBookmark::positionInParent(parent.address()) , 0, item->parent());
    else //parent is root
        return createIndex( 0, 0, item->parent());
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
            return static_cast<TreeItem *>(parent.internalPointer())->childCount();
    else //root case
    {
        return 1;
    }
}

int BookmarkModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QModelIndex BookmarkModel::bookmarkToIndex(KBookmark bk)
{
    return createIndex( KBookmark::positionInParent(bk.address()), 0, rootItem->treeItemForBookmark(bk));
}

void BookmarkModel::emitDataChanged(KBookmark bk)
{
    QModelIndex index = bookmarkToIndex(bk);
    emit dataChanged(index, index );
}

QMimeData * BookmarkModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *mimeData = new QMimeData;
    //FIXME use KBookmark::addToMimeData (or whatever KBookmarkDrag is ported to.)
    //FIXME for now insert bogus things
    mimeData->setData("application/x-xbel", "XXXX");
    return mimeData;
}

Qt::DropActions BookmarkModel::supportedDropActions () const
{
    //FIXME should return Qt::MoveAction | Qt::CopyAction
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList BookmarkModel::mimeTypes () const
{
    //FIXME reuse KBookmark::mimeTypes (or something like that)
    QStringList list;
    list.append("application/x-xbel");
    list.append("text/uri-list");
    list.append("text/plain");
    return list;
}

bool BookmarkModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
    QModelIndex idx;
    // idx is the index on which the data was dropped
    // work around stupid design of the qt api:
    if(row == -1)   
        idx = parent;
    else
        idx = index(row, column, parent);
    KBookmark bk = static_cast<TreeItem *>(idx.internalPointer())->bookmark();
    QString addr = bk.address();
    if(bk.isGroup())
        addr += "/0"; //FIXME internal representation
    KCommand * mcmd = CmdGen::itemsMoved(KEBApp::self()->selectedBookmarks() , addr, false);
    CmdHistory::self()->didCommand(mcmd);
    return true;
}

#include "bookmarkmodel.moc"
