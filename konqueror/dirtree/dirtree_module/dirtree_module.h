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

#include <konq_treemodule.h>
#include <kfileitem.h>
#include <qpixmap.h>

class KonqTree;
class KonqTreeItem;
class KonqDirTreeItem;
class KonqPropsView;

class KonqDirTreeModule : public QObject, public KonqTreeModule
{
    Q_OBJECT
public:
    KonqDirTreeModule( KonqTree * parentTree );
    virtual ~KonqDirTreeModule();

    virtual void addTopLevelItem( KonqTreeTopLevelItem * item );

    virtual void openTopLevelItem( KonqTreeTopLevelItem * item );

    virtual void followURL( const KURL & url );

    // Called by KonqDirTreeItem
    void openSubFolder( KonqTreeItem *item );
    void addSubDir( KonqTreeItem *item );
    void removeSubDir( KonqTreeItem *item, bool childrenonly = false );

private slots:
    void slotNewItems( const KFileItemList & );
    void slotRefreshItems( const KFileItemList & );
    void slotDeleteItem( KFileItem *item );
    void slotRedirection( const KURL & oldUrl, const KURL & newUrl );
    void slotListingStopped();

private:
    //KonqTreeItem * findDir( const KURL &_url );
    void listDirectory( KonqTreeItem *item );
    KURL::List selectedUrls();

    // URL -> item
    QDict<KonqTreeItem> m_dictSubDirs;

    // The dirlister - having only one prevents opening two subdirs at the same time,
    // but it's necessary for the update feature.... if we want two openings on the
    // same tree, it requires a major kdirlister improvement (rather as a subclass).
    KonqDirLister * m_dirLister;

    QList<KonqTreeItem> m_lstPendingOpenings;

    KURL m_selectAfterOpening;

    KonqTreeTopLevelItem * m_topLevelItem;

    KonqPropsView * s_defaultViewProps;
    KonqPropsView * m_pProps;
};


#endif
