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

#ifndef bookmark_module_h
#define bookmark_module_h

#include <qobject.h>
#include <konq_sidebartreemodule.h>
#include <kbookmark.h>
class KonqSidebarBookmarkItem;

/**
 * This module displays bookmarks in the tree
 */
class KonqSidebarBookmarkModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarBookmarkModule( KonqSidebarTree * parentTree );
    virtual ~KonqSidebarBookmarkModule();

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );

protected slots:
    void slotBookmarksChanged( const QString & );

protected:
    void fillListView();
    void fillGroup( KonqSidebarTreeItem * parentItem, KBookmarkGroup group );
    KonqSidebarBookmarkItem * findByAddress( const QString & address ) const;

private:
    KonqSidebarTreeTopLevelItem * m_topLevelItem;
    KonqSidebarBookmarkItem * m_rootItem;
};

#endif
