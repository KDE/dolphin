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

#ifndef konq_treeitem_h
#define konq_treeitem_h

#include <qlistview.h>
#include <kurl.h>
#include <konq_dirlister.h>
class QPainter;
class QDragObject;
class KonqTree;
class KonqTreeItem;
class KonqTreeModule;
class KonqTreeTopLevelItem;

/**
 * The base class for any item in the tree.
 * Items belonging to a given module are created and managed by the module,
 * but they should all be KonqTreeItems, for the event handling in KonqTree.
 */
class KonqTreeItem : public QListViewItem
{
public:
    // Create an item under another one
    KonqTreeItem( KonqTreeItem *parentItem, KonqTreeTopLevelItem *topLevelItem );

    void initItem( KonqTreeTopLevelItem *topLevelItem );

    virtual ~KonqTreeItem() {}

    // Whether the item accepts a drop consisting in those @p formats
    virtual bool acceptsDrops( const QStrList & ) { return false; }

    // Handle a drop on this item. If you didn't want it, you shouln't
    // have return true in acceptsDrops :)
    virtual void drop( QDropEvent * ) {}

    // Create a drag object from this item.
    virtual QDragObject * dragObject( QWidget * parent, bool move = false ) = 0;

    virtual void middleButtonPressed();
    virtual void rightButtonPressed() = 0;

    virtual void paste() {}
    virtual void trash() {}
    virtual void del() {}
    virtual void shred() {}
    virtual void rename() {}

    // The URL to open when this link is clicked
    virtual KURL externalURL() const = 0;

    // The mimetype to use when this link is clicked
    // If unknown, return QString::null, konq will determine the mimetype itself
    virtual QString externalMimeType() const { return QString::null; }

    // overwrite this if you want a tooltip shown on your item
    virtual QString toolTipText() const { return QString::null; }

    // Called when this item is selected
    // Reimplement, and call tree()->part()->extension()->enableActions(...)
    virtual void itemSelected() = 0;

    // Basically, true for directories and toplevel items
    void setListable( bool b ) { m_bListable = b; }
    bool isListable() const { return m_bListable; }

    // Whether clicking on the item should open the "external URL" of the item
    void setClickable( bool b ) { m_bClickable = b; }
    bool isClickable() const { return m_bClickable; }

    // Whether the item is a toplevel item
    virtual bool isTopLevelItem() const { return false; }

    KonqTreeTopLevelItem * topLevelItem() const { return m_topLevelItem; }

    // returns the module associated to our toplevel item
    KonqTreeModule * module() const;

    // returns the tree inside which this item is
    KonqTree *tree() const;

protected:
    // Create an item at the toplevel - only for toplevel items -> protected
    KonqTreeItem( KonqTree *parent, KonqTreeTopLevelItem *topLevelItem );

    KonqTreeTopLevelItem *m_topLevelItem;
    bool m_bListable:1;
    bool m_bClickable:1;
};

#endif
