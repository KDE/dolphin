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

#ifndef konq_treemodule_h
#define konq_treemodule_h

#include <qobject.h>
#include <konq_tree.h>
class QDragObject;
class KonqTreeItem;
class KonqTreeTopLevelItem;
class KonqTree;

/**
 * The base class for KonqTree Modules. It defines the interface
 * between the generic KonqTree and the particular modules
 * (directory tree, history, bookmarks, ...)
 */
class KonqTreeModule
{
public:
    KonqTreeModule( KonqTree * parentTree )
        : m_pTree( parentTree ) {}
    virtual ~KonqTreeModule() {}

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem( KonqTreeTopLevelItem * item ) = 0;

    // Open this toplevel item - you don't need to reimplement if
    // you create the item's children right away
    virtual void openTopLevelItem( KonqTreeTopLevelItem * ) {}

    // Follow a URL opened in another view - only implement if the module
    // has anything to do with URLs
    virtual void followURL( const KURL & ) {}

    KonqTree *tree() const { return m_pTree; }

protected:
    KonqTree * m_pTree;
};

#endif
