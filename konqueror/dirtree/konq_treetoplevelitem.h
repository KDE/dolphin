/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef konq_treetoplevelitem_h
#define konq_treetoplevelitem_h

#include "konq_treeitem.h"
#include "konq_treemodule.h"

/**
 * Each toplevel item (created from a desktop file)
 * points to the module that handles it
  --> this doesn't prevent the same module from handling multiple toplevel items,
  but we don't do that currently.
 */
class KonqTreeTopLevelItem
{
public:
    /**
     * Create a toplevel item.
     * @param item the corresponding item in the listview
     * @param module the module handling this toplevel item
     * @param path the path to the desktop file that was the reason for creating this item
     */
    KonqTreeTopLevelItem( KonqTreeItem *item, KonqTreeModule * module, const QString & path )
        : m_item(item), m_module(module), m_path(path) {}
    KonqTreeTopLevelItem() : m_item(0), m_module(0) {} // for QValueList

    KonqTreeItem *item() const { return m_item; }
    KonqTreeModule *module() const { return m_module; }
    QString path() const { return m_path; }

protected:
    KonqTreeItem *m_item;
    KonqTreeModule *m_module;
    QString m_path;
};

#endif
