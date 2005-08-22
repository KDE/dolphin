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

#include "treeitem.h"
#include <kdebug.h>

TreeItem::TreeItem(KBookmark bk, TreeItem * parent)
    : mparent(parent), mbk(bk)
{
    init = false;
#ifdef DEBUG_STUPID_QT
    deleted = false;
#endif
}

TreeItem::~TreeItem()
{
    qDeleteAll(children);
    children.clear();
}

TreeItem * TreeItem::child(int row)
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kdFatal()<<"child for deleted "<<endl;
#endif
    if(!init)
        initChildren();
    return children[row];
}

int TreeItem::childCount()
{
    if(!init)
        initChildren();
    return children.count();
}

TreeItem * TreeItem::parent()
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kdFatal()<<"parent for deleted "<<endl;
#endif
    return mparent;
}

#ifdef DEBUG_STUPID_QT
void TreeItem::markDelete()
{
    deleted = true;
    QList<TreeItem *>::iterator it, end;
    end = children.end();
    for(it = children.begin(); it != end; ++it)
    {
        (*it)->markDelete();
    }
}
#endif

void TreeItem::insertChildren(int first, int last)
{
    // Find child number last
    KBookmarkGroup parent = bookmark().toGroup();
    KBookmark child = parent.first();
    for(int i=0; i < last; ++i)
        child = parent.next(child);
    
    //insert children
    int i = last;
    do
    {
        children.insert(i, new TreeItem(child, this));
        child = parent.previous(child);
        --i;
    } while(i >= first);

}
void TreeItem::deleteChildren(int first, int last)
{
    kdDebug()<<"deleteChildren of "<<bookmark().address()<<" "<<first<<" "<<last<<endl;
    QList<TreeItem *>::iterator firstIt, lastIt, it;
    firstIt = children.begin() + first;
    lastIt = children.begin() + last + 1;
    for( it = firstIt; it != lastIt; ++it)
    {
        kdDebug()<<"removing child "<<(*it)->bookmark().address()<<endl;
#ifndef DEBUG_STUPID_QT
        delete *it;
#else
        (*it)->markDelete();
#endif
    }
    children.erase(firstIt, lastIt);
}

KBookmark TreeItem::bookmark()
{
#ifdef DEBUG_STUPID_QT
    if(deleted)
        kdFatal()<<"child for deleted "<<endl;
#endif
    return mbk;
}

void TreeItem::initChildren()
{
    init = true;
    if(mbk.isGroup())
    {
        KBookmarkGroup parent = mbk.toGroup();
        for(KBookmark child = parent.first(); child.hasParent(); child = parent.next(child) )
        {
            TreeItem * item = new TreeItem(child, this);
            children.append(item);
        }
    }
}

TreeItem * TreeItem::treeItemForBookmark(KBookmark bk)
{
    if(bk.address() == mbk.address())
        return this;
    QString commonParent = KBookmark::commonParent(bk.address(), mbk.address());
    if(commonParent == mbk.address()) //mbk is a parent of bk
    {
        QList<TreeItem *>::const_iterator it, end;
        end = children.constEnd();
        for( it = children.constBegin(); it != end; ++it)
        {
            KBookmark child = (*it)->bookmark();
            if( KBookmark::commonParent(child.address(), bk.address()) == child.address())
                    return (*it)->treeItemForBookmark(bk);
        }
        return 0;
    }
    else
    {
        if(parent() == 0)
            return 0;
        return parent()->treeItemForBookmark(bk);
    }
}

