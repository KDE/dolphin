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
#include <kdebug.h>

int BookmarkModel::count = 0;
BookmarkModel* BookmarkModel::s_bookmarkModel = 0L;

void BookmarkModel::resetModel()
{
    reset();
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
    if( bk.address() != ""  )
    {
        if( bk.isGroup())
        {
            if(index.column() == 0 || index.column() == 2)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        }
        else
            if(index.column() < 3)
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool BookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.isValid() && role == Qt::EditRole)
    {
        //FIXME don't create a commmand if still the same
        // and ignore if name column and empty
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
    {
        KBookmark bk = static_cast<TreeItem *>(parent.internalPointer())->bookmark();
        return childCount(bk);
    }
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

int BookmarkModel::childCount(KBookmark bk)
{
    if(!bk.isGroup())
        return 0;
    KBookmark child = bk.toGroup().first();
    int i = 0;
    while(child.hasParent())
    {
        ++i;
        child = bk.toGroup().next(child);
    }
    return i;
}

void BookmarkModel::emitDataChanged(KBookmark bk)
{
    QModelIndex index = bookmarkToIndex(bk);
    emit dataChanged(index, index );
}

#include "bookmarkmodel.moc"
