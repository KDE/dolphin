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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef konq_treetoplevelitem_h
#define konq_treetoplevelitem_h

#include "konq_sidebartreeitem.h"

class QMimeData;
class QStrList;
class Q3DragObject;
class KonqSidebarTreeModule;

/**
 * Each toplevel item (created from a desktop file)
 * points to the module that handles it
  --> this doesn't prevent the same module from handling multiple toplevel items,
  but we don't do that currently.
 */
class KonqSidebarTreeTopLevelItem : public KonqSidebarTreeItem
{
public:
    /**
     * Create a toplevel toplevel-item :)
     * @param module the module handling this toplevel item
     * @param path the path to the desktop file that was the reason for creating this item
     */
    KonqSidebarTreeTopLevelItem( KonqSidebarTree *parent, KonqSidebarTreeModule * module, const QString & path )
        : KonqSidebarTreeItem(parent, 0L), m_module(module), m_path(path), m_bTopLevelGroup(false) { init(); }

    /**
     * Create a toplevel-item under a toplevel group
     * @param module the module handling this toplevel item
     * @param path the path to the desktop file that was the reason for creating this item
     */
    KonqSidebarTreeTopLevelItem( KonqSidebarTreeItem *parentItem, KonqSidebarTreeModule * module, const QString & path )
        : KonqSidebarTreeItem( parentItem, 0L), m_module(module), m_path(path), m_bTopLevelGroup(false) { init(); }

    void init();

    virtual bool acceptsDrops( const Q3StrList & formats );
    virtual void drop( QDropEvent * ev );
    virtual bool populateMimeData( QMimeData* mimeData, bool move );
    virtual void middleButtonClicked();
    virtual void rightButtonPressed();

    virtual void paste();
    virtual void trash();
    virtual void del();
    virtual void shred();
    virtual void rename(); // start a rename operation
    virtual void rename( const QString & name ); // do the actual renaming

    virtual void setOpen( bool open );

    // Whether the item is a toplevel item - true
    virtual bool isTopLevelItem() const { return true; }

    virtual KUrl externalURL() const { return m_externalURL; }

    virtual QString toolTipText() const;

    virtual void itemSelected();

    // The module should call this for each toplevel item that is passed to it
    // unless it calls setClickable(false)
    void setExternalURL( const KUrl & url ) { m_externalURL = url; }

    // Whether the item is a toplevel group. [Only matters for dnd]
    void setTopLevelGroup( bool b ) { m_bTopLevelGroup = b; }
    bool isTopLevelGroup() const { return m_bTopLevelGroup; }

    // The module that handles the subtree below this toplevel item
    KonqSidebarTreeModule *module() const { return m_module; }

    // The path to the desktop file responsible for this toplevel item
    QString path() const { return m_path; }

protected:
    void delOperation( int method );
    KonqSidebarTreeModule *m_module;
    QString m_path;
    QString m_comment;
    KUrl m_externalURL;
    bool m_bTopLevelGroup;
};

#endif
