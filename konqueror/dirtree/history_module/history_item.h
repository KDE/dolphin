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
    KonqHistoryItem( const KonqHistoryEntry *entry,
		     KonqTreeItem *parentItem,
		     KonqTreeTopLevelItem *topLevelItem );
    ~KonqHistoryItem();

    virtual void setOpen( bool open );

    // just to make the baseclass happy (pure-virtual methods we don't support)
    virtual bool acceptsDrops( const QStrList & /*formats*/ ) { return false; }
    virtual void drop( QDropEvent * ) {}

    virtual void rightButtonPressed();

    virtual void itemSelected();

    // The URL to open when this link is clicked
    virtual KURL externalURL() const { return m_entry->url; }
    const KURL& url() const { return m_entry->url; } // a faster one

    QString host() const { return m_entry->url.host(); }
    QString path() const { return m_entry->url.path(); }

    void update( const KonqHistoryEntry *entry );

    virtual QDragObject * dragObject( QWidget * parent, bool move = false );

private:
    const KonqHistoryEntry *m_entry;

};

class KonqHistoryGroupItem : public KonqTreeItem
{
public:

    KonqHistoryGroupItem( const QString& host, KonqTreeTopLevelItem * );

    /**
     * removes itself and all its children from the history (not just the view)
     */
    void remove();

    KonqHistoryItem * findChild( const KonqHistoryEntry *entry ) const;

    virtual void rightButtonPressed();


    // we don't support the following
    bool acceptsDrops( const QStrList& ) { return false; }
    virtual void drop( QDropEvent * ) {}
    virtual QDragObject * dragObject( QWidget *, bool ) { return 0L; }
    virtual KURL externalURL() const { return KURL(); }
    virtual void itemSelected();

private:
    QString m_host;

};


#endif // HISTORY_ITEM_H
