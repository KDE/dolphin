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
#include <konq_treemodule.h>
#include <kbookmark.h>
class KonqBookmarkItem;

/**
 * This module displays bookmarks in the tree
 */
class KonqBookmarkModule : public QObject, public KonqTreeModule
{
    Q_OBJECT
public:
    KonqBookmarkModule( KonqTree * parentTree );
    virtual ~KonqBookmarkModule();

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem( KonqTreeTopLevelItem * item );

protected slots:
    void slotBookmarksChanged( const QString & );

protected:
    void fillListView();
    void fillGroup( KonqTreeItem * parentItem, KBookmarkGroup group );
    KonqBookmarkItem * findByAddress( const QString & address ) const;

private:
    KonqTreeTopLevelItem * m_topLevelItem;
    KonqBookmarkItem * m_rootItem;
};

#endif
