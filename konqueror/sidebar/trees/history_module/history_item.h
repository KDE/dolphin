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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef HISTORY_ITEM_H
#define HISTORY_ITEM_H

#include <kurl.h>
#include <konq_historymgr.h>

#include "konq_sidebartreeitem.h"

class QDropEvent;
class QPainter;
class KonqSidebarHistorySettings;

class KonqSidebarHistoryItem : public KonqSidebarTreeItem
{
public:
    KonqSidebarHistoryItem( const KonqHistoryEntry& entry,
		     KonqSidebarTreeItem *parentItem,
		     KonqSidebarTreeTopLevelItem *topLevelItem );
    ~KonqSidebarHistoryItem();

    virtual void rightButtonPressed();

    virtual void itemSelected();

    // The URL to open when this link is clicked
    virtual KUrl externalURL() const { return m_entry.url; }
    const KUrl& url() const { return m_entry.url; } // a faster one
    virtual QString toolTipText() const;

    QString host() const { return m_entry.url.host(); }
    QString path() const { return m_entry.url.path(); }

    const QDateTime& lastVisited() const { return m_entry.lastVisited; }

    void update( const KonqHistoryEntry& entry );
    const KonqHistoryEntry& entry() const { return m_entry; }

    virtual bool populateMimeData( QMimeData* mimeData, bool move );

    virtual QString key( int column, bool ascending ) const;

    static void setSettings( KonqSidebarHistorySettings *s ) { s_settings = s; }

    virtual void paintCell( QPainter *, const QColorGroup & cg, int column,
			    int width, int alignment );

private:
    const KonqHistoryEntry m_entry;
    static KonqSidebarHistorySettings *s_settings;

};

class KonqSidebarHistoryGroupItem : public KonqSidebarTreeItem
{
public:

    KonqSidebarHistoryGroupItem( const KUrl& url, KonqSidebarTreeTopLevelItem * );

    /**
     * removes itself and all its children from the history (not just the view)
     */
    void remove();

    KonqSidebarHistoryItem * findChild( const KonqHistoryEntry& entry ) const;

    virtual void rightButtonPressed();

    virtual void setOpen( bool open );

    virtual QString key( int column, bool ascending ) const;

    void itemUpdated( KonqSidebarHistoryItem *item );

    bool hasFavIcon() const { return m_hasFavIcon; }
    void setFavIcon( const QPixmap& pix );

    virtual bool populateMimeData( QMimeData* mimeData, bool move );
    virtual void itemSelected();

    // we don't support the following of KonqSidebarTreeItem
    bool acceptsDrops( const QStrList& ) { return false; }
    virtual void drop( QDropEvent * ) {}
    virtual KUrl externalURL() const { return KUrl(); }

private:
    bool m_hasFavIcon;
    const KUrl m_url;
    QDateTime m_lastVisited;

};


#endif // HISTORY_ITEM_H
