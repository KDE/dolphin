/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org

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

#ifndef HISTORY_ITEM_H
#define HISTORY_ITEM_H

#include <kurl.h>
#include <konq_historymgr.h>

#include "konq_treeitem.h"

class QDropEvent;

class KonqHistoryItem : public KonqTreeItem
{
public:
    KonqHistoryItem( KonqTree *parent, KonqTreeItem *parentItem,
		     KonqTreeTopLevelItem *topLevelItem,
		     const KonqHistoryEntry *entry );
    KonqHistoryItem( KonqTree *parent, KonqTreeTopLevelItem *topLevelItem,
		     const KonqHistoryEntry *entry );
    ~KonqHistoryItem();

    virtual void setOpen( bool open );

    // just to make the baseclass happy (pure-virtual methods we don't support)
    virtual bool acceptsDrops( const QStrList & /*formats*/ ) { return false; }
    virtual void drop( QDropEvent * ) {}

    virtual void rightButtonPressed();

    virtual void itemSelected();

    // The URL to open when this link is clicked
    virtual KURL externalURL() const { return m_entry->url; }

    void update( const KonqHistoryEntry *entry );

    virtual QDragObject * dragObject( QWidget * parent, bool move = false );
    
private:
    const KonqHistoryEntry *m_entry;

};

#endif // HISTORY_ITEM_H
