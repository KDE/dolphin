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

#ifndef dirtree_module_h
#define dirtree_module_h

#include <konq_sidebartreemodule.h>
#include <kfileitem.h>
#include <qpixmap.h>

class KonqSidebarTree;
class KonqSidebarTreeItem;
class KonqSidebarDirTreeItem;
class KonqPropsView;

class KonqSidebarDirTreeModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarDirTreeModule( KonqSidebarTree * parentTree );
    virtual ~KonqSidebarDirTreeModule();

    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );

    virtual void openTopLevelItem( KonqSidebarTreeTopLevelItem * item );

    virtual void followURL( const KURL & url );

    // Called by KonqSidebarDirTreeItem
    void openSubFolder( KonqSidebarTreeItem *item );
    void addSubDir( KonqSidebarTreeItem *item );
    void removeSubDir( KonqSidebarTreeItem *item, bool childrenonly = false );

private slots:
    void slotNewItems( const KFileItemList & );
    void slotRefreshItems( const KFileItemList & );
    void slotDeleteItem( KFileItem *item );
    void slotRedirection( const KURL & oldUrl, const KURL & newUrl );
    void slotListingStopped();

private:
    //KonqSidebarTreeItem * findDir( const KURL &_url );
    void listDirectory( KonqSidebarTreeItem *item );
    KURL::List selectedUrls();

    // URL -> item
    QDict<KonqSidebarTreeItem> m_dictSubDirs;

    // The dirlister - having only one prevents opening two subdirs at the same time,
    // but it's necessary for the update feature.... if we want two openings on the
    // same tree, it requires a major kdirlister improvement (rather as a subclass).
    KonqDirLister * m_dirLister;

    QList<KonqSidebarTreeItem> m_lstPendingOpenings;

    KURL m_selectAfterOpening;

    KonqSidebarTreeTopLevelItem * m_topLevelItem;

    KonqPropsView * s_defaultViewProps;
    KonqPropsView * m_pProps;
};


#endif
