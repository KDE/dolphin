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

class KonqTree;
class KonqTreeItem;
class KonqDirTreeItem;

class KonqDirTreeModule : public KonqTreeModule
{
    Q_OBJECT
public:
    DirTreeModule( KonqTree * parentTree, const char * name = 0 );
    virtual ~DirTreeModule() {}

    virtual void clearAll();

    // Used by copy() and cut()
    virtual QDragObject * dragObject( QWidget * parent, bool move = false );
    virtual void paste();
    virtual void trash();
    virtual void del();
    virtual void shred();
    /**
     * Called when an item in this module is selected
     * Emit the appropriate enableAction signals.
     */
    virtual void slotSelectionChanged();

    virtual void mmbClicked( KonqTreeItem * item );

    // Called by KonqDirTreeItem
    void openSubFolder( KonqTreeItem *item, KonqTreeItem *topLevel );
    void addSubDir( KonqTreeItem *item, KonqTreeItem *topLevel, const KURL &url );
    void removeSubDir( KonqTreeItem *item, KonqTreeItem *topLevel, const KURL &url );


private slots:
    void slotNewItems( const KFileItemList & );
    void slotDeleteItem( KFileItem *item );
    void slotRedirection( const KURL & );

    void slotListingStopped();

private:
    void openDirectory( const KURL & url, bool keep );

    QMap<KURL, KonqDirTreeItem*> *subDirMap; // TODO get rid of "*"
    KonqDirLister *m_dirLister; // TODO get rid of "*"
    QMap<KURL,KonqDirTreeItem *> *m_mapSubDirs; // TODO get rid of "*"
    KURL::List *m_lstPendingURLs; // TODO get rid of "*"
    KURL m_currentlyListedURL; // always the same as m_dirLister->url(), but used for redirections

    KURL m_selectAfterOpening;

    static KonqPropsView * s_defaultViewProps;
    KonqPropsView * m_pProps;
};


#endif
