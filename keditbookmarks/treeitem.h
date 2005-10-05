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

#ifndef __treeitem_h
#define __treeitem_h

#include <QList>
#include <kbookmark.h>

//#define DEBUG_STUPID_QT

class TreeItem
{
public:
    TreeItem(KBookmark bk, TreeItem * parent);
    ~TreeItem();
    TreeItem * child(int row);
    TreeItem * parent();

    void insertChildren(int first, int last);
    void deleteChildren(int first, int last);
    void moveChildren(int first, int last, TreeItem * newParent, int position);
    KBookmark bookmark();
    int childCount();
    TreeItem * treeItemForBookmark(KBookmark bk);
private:
#ifdef DEBUG_STUPID_QT
    void markDelete();
    bool deleted;
#endif
    void initChildren();
    bool init;
    //FIXME use something else
    QList<TreeItem *> children;
    TreeItem * mparent;
    KBookmark mbk;
};
#endif
