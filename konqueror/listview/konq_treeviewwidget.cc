/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002 Michael Brade <brade@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_listview.h"
#include "konq_treeviewwidget.h"

#include <kdirlister.h>
#include <kdebug.h>


template class QDict<KonqListViewDir>;

//////////////////////////////////////////////
KonqTreeViewWidget::KonqTreeViewWidget( KonqListView *parent, QWidget *parentWidget)
: KonqBaseListViewWidget(parent,parentWidget)
{
   kdDebug(1202) << "+KonqTreeViewWidget" << endl;

   setRootIsDecorated( true );
   setTreeStepSize( 20 );

   connect( m_dirLister, SIGNAL( completed( const KURL & ) ),
            this, SLOT( slotCompleted( const KURL & ) ) );
   connect( m_dirLister, SIGNAL( clear( const KURL & ) ),
            this, SLOT( slotClear( const KURL & ) ) );
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
   kdDebug(1202) << "-KonqTreeViewWidget" << endl;

   // Remove all items
   clear();
   // Clear dict
   m_dictSubDirs.clear();
}

bool KonqTreeViewWidget::openURL( const KURL &url )
{
   if ( m_pBrowserView->extension()->urlArgs().reload )
   {
      QDictIterator<KonqListViewDir> it( m_dictSubDirs );
      for (; it.current(); ++it )
         if ( it.current()->isOpen() )
            m_urlsToReload.append( it.current()->url( -1 ) );

      // Someone could press reload while the listing is still in progess
      // -> move the items that are not opened yet to m_urlsToReload.
      // We don't need to check for doubled items since remove() removes
      // all occurances.
      m_urlsToReload += m_urlsToOpen;
      m_urlsToOpen.clear();
   }

   return KonqBaseListViewWidget::openURL( url );
}

void KonqTreeViewWidget::saveState( QDataStream &stream )
{
    QStringList openDirList;

    QDictIterator<KonqListViewDir> it( m_dictSubDirs );
    for (; it.current(); ++it )
        if ( it.current()->isOpen() )
            openDirList.append( it.current()->url( -1 ) );

    stream << openDirList;
    KonqBaseListViewWidget::saveState( stream );
}

void KonqTreeViewWidget::restoreState( QDataStream &stream )
{
    stream >> m_urlsToOpen;
    KonqBaseListViewWidget::restoreState( stream );
}

void KonqTreeViewWidget::addSubDir( KonqListViewDir* _dir )
{
   m_dictSubDirs.insert( _dir->url(-1), _dir );
}

void KonqTreeViewWidget::removeSubDir( const KURL & _url )
{
   m_dictSubDirs.remove( _url.url(-1) );
}

void KonqTreeViewWidget::slotCompleted()
{
   // This is necessary because after reloading it could happen that a
   // previously opened subdirectory was deleted and this would still
   // be queued for opening.
   m_urlsToReload.clear();
   m_urlsToOpen.clear();

   KonqBaseListViewWidget::slotCompleted();
}

void KonqTreeViewWidget::slotCompleted( const KURL & _url )
{
    // do nothing if the view itself is finished
    if ( m_url.cmp( _url, true ) )
        return;

    KonqListViewDir *dir = m_dictSubDirs[ _url.url(-1) ];
    if ( dir )
        dir->setComplete( true );
    else
        kdWarning() << "KonqTreeViewWidget::slotCompleted : dir " << _url.url(-1) << " not found in dict!" << endl;

    if ( !viewport()->isUpdatesEnabled() )
    {
        viewport()->setUpdatesEnabled( true );
        setUpdatesEnabled( true );
        triggerUpdate();
    }
}

void KonqTreeViewWidget::slotClear()
{
   kdDebug(1202) << k_funcinfo << endl;

   m_dictSubDirs.clear();
   KonqBaseListViewWidget::slotClear();
}

void KonqTreeViewWidget::slotClear( const KURL & _url )
{
   kdDebug(1202) << "#################### MY ONE!!!" << k_funcinfo << endl;

   // we are allowed to delete the whole content since the opening of
   // subdirs happens level per level.

   QListViewItem *item = m_dictSubDirs[_url.url(-1)];
   Q_ASSERT( item );

   QDictIterator<KonqListViewDir> it( m_dictSubDirs );
   for ( ; it.current(); ++it )
      if ( _url.isParentOf( it.current()->url(0) ) )
         m_dictSubDirs.remove( _url.url(-1) );

   QListViewItemIterator qit( item );
   QPtrList<QListViewItem> *lst = new QPtrList<QListViewItem>;
   lst->setAutoDelete( true );

   for ( ; qit.current(); ++qit )
   {
      // delete the item from the counts for the statusbar
//      KFileItem* item = static_cast<KonqListViewItem*>(qit.current())->item();
//      treeView->m_pBrowserView->deleteItem( item );
      lst->append( qit.current() );
   }

   delete lst;

   m_urlsToOpen.remove( _url.url(-1) );
   m_urlsToReload.remove( _url.url(-1) );
}

void KonqTreeViewWidget::slotNewItems( const KFileItemList & entries )
{
    // Find parent item - it's the same for all the items
    QPtrListIterator<KFileItem> kit( entries );
    KURL dir( (*kit)->url() );
    dir.setFileName( "" );

    KonqListViewDir * parentDir = 0L;
    if ( !m_url.cmp( dir, true ) ) // ignore trailing slash
        parentDir = m_dictSubDirs[ dir.url(-1) ];

    for ( ; kit.current(); ++kit )
    {
        KonqListViewDir *dirItem = 0;
        KonqListViewItem *fileItem = 0;

        if ( parentDir ) // adding under a directory item
        {
            if ( (*kit)->isDir() )
                dirItem = new KonqListViewDir( this, parentDir, *kit );
            else
                fileItem = new KonqListViewItem( this, parentDir, *kit );
        }
        else   // adding on the toplevel
        {
            if ( (*kit)->isDir() )
                dirItem = new KonqListViewDir( this, *kit );
            else
                fileItem = new KonqListViewItem( this, *kit );
        }

        if ( !m_itemFound )
        {
            if ( fileItem && fileItem->text(0) == m_itemToGoTo )
            {
                setCurrentItem( fileItem );
                m_itemFound = true;
            }
            else if ( dirItem && dirItem->text(0) == m_itemToGoTo )
            {
                setCurrentItem(dirItem);
                m_itemFound=true;
            }
        }

        if ( fileItem && !(*kit)->isMimeTypeKnown() )
            m_pBrowserView->lstPendingMimeIconItems().append( fileItem );

        if ( dirItem )
        {
            QString u = (*kit)->url().url( 0 );
            if ( m_urlsToOpen.remove( u ) )
                dirItem->open( true, false );
            else if ( m_urlsToReload.remove( u ) )
                dirItem->open( true, true );
        }
    }

    if ( !viewport()->isUpdatesEnabled() )
    {
        viewport()->setUpdatesEnabled( true );
        setUpdatesEnabled( true );
        triggerUpdate();
    }

    // counts for the statusbar
    m_pBrowserView->newItems( entries );
    slotUpdateBackground();
}

void KonqTreeViewWidget::slotDeleteItem( KFileItem *_fileItem )
{
    QString url = _fileItem->url().url( 0 );
    m_urlsToOpen.remove( url );
    m_urlsToReload.remove( url );

    // Check if this item is in m_dictSubDirs, and if yes, then remove it
    removeSubDir( _fileItem->url().url(-1) );

    KonqBaseListViewWidget::slotDeleteItem( _fileItem );
}

void KonqTreeViewWidget::openSubFolder( KonqListViewDir* _dir, bool _reload )
{
   m_dirLister->openURL( _dir->item()->url(), true /* keep existing data */, _reload );
   slotUpdateBackground();
}

void KonqTreeViewWidget::stopListingSubFolder( KonqListViewDir* _dir )
{
   m_dirLister->stop( _dir->item()->url() );
   slotUpdateBackground();
}

#include "konq_treeviewwidget.moc"
