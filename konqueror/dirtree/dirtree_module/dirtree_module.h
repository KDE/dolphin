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

class KonqTree;
class KonqTreeItem;
class KonqDirTreeItem;
class KonqPropsView;

class KonqDirTreeModule : public QObject, public KonqTreeModule
{
    Q_OBJECT
public:
    KonqDirTreeModule( KonqTree * parentTree );
    virtual ~KonqDirTreeModule() {}

    virtual void addTopLevelItem( KonqTreeTopLevelItem * item );

    virtual void clearAll();

    // Used by copy() and cut()
    virtual QDragObject * dragObject( QWidget * parent, bool move = false );
    virtual void paste();
    virtual void trash();
    virtual void del();
    virtual void shred();

    // Called by KonqDirTreeItem
    void openSubFolder( KonqDirTreeItem *item );
    void addSubDir( KonqDirTreeItem *item, const KURL &url );
    void removeSubDir( KonqDirTreeItem *item, const KURL &url );

private slots:
    void slotNewItems( const KFileItemList & );
    void slotDeleteItem( KFileItem *item );
    void slotRedirection( const KURL & );
    void slotListingStopped();

private:
    void openDirectory( const KURL & url, bool keep );
    KURL::List selectedUrls();

    KonqDirLister *m_dirLister;
    QMap<KURL,KonqDirTreeItem *> m_mapSubDirs;
    KURL::List m_lstPendingURLs;
    KURL m_currentlyListedURL; // always the same as m_dirLister->url(), but used for redirections

    KURL m_selectAfterOpening;
    //KonqTreeTopLevelItem * m_topLevelItem;

    KonqPropsView * s_defaultViewProps;
    KonqPropsView * m_pProps;
};


#endif
