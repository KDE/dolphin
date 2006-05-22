/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002, 2004 Michael Brade <brade@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_listview.h"
#include "konq_treeviewwidget.h"

#include <kdirlister.h>
#include <kdebug.h>

template class Q3Dict<KonqListViewDir>;


KonqTreeViewWidget::KonqTreeViewWidget( KonqListView *parent, QWidget *parentWidget)
   : KonqBaseListViewWidget( parent, parentWidget )
{
   kDebug(1202) << "+KonqTreeViewWidget" << endl;

   setRootIsDecorated( true );
   setTreeStepSize( 20 );

   connect( m_dirLister, SIGNAL( completed( const KUrl & ) ),
            this, SLOT( slotCompleted( const KUrl & ) ) );
   connect( m_dirLister, SIGNAL( clear( const KUrl & ) ),
            this, SLOT( slotClear( const KUrl & ) ) );
   connect( m_dirLister, SIGNAL( redirection( const KUrl &, const KUrl & ) ),
            this, SLOT( slotRedirection( const KUrl &, const KUrl & ) ) );
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
   kDebug(1202) << "-KonqTreeViewWidget" << endl;

   // Remove all items
   clear();
   // Clear dict
   m_dictSubDirs.clear();
}

bool KonqTreeViewWidget::openURL( const KUrl &url )
{
   //kDebug(1202) << k_funcinfo << url.prettyUrl() << endl;

   if ( m_pBrowserView->extension()->urlArgs().reload )
   {
      Q3DictIterator<KonqListViewDir> it( m_dictSubDirs );
      for (; it.current(); ++it )
         if ( it.current()->isOpen() )
            m_urlsToReload.append( it.current()->url( KUrl::RemoveTrailingSlash ) );

      // Someone could press reload while the listing is still in progess
      // -> move the items that are not opened yet to m_urlsToReload.
      // We don't need to check for doubled items since remove() removes
      // all occurrences.
      m_urlsToReload += m_urlsToOpen;
      m_urlsToOpen.clear();
   }

   return KonqBaseListViewWidget::openURL( url );
}

void KonqTreeViewWidget::saveState( QDataStream &stream )
{
    QStringList openDirList;

    Q3DictIterator<KonqListViewDir> it( m_dictSubDirs );
    for (; it.current(); ++it )
    {
        if ( it.current()->isOpen() )
            openDirList.append( it.current()->url( KUrl::RemoveTrailingSlash ) );
    }

    stream << openDirList;
    KonqBaseListViewWidget::saveState( stream );
}

void KonqTreeViewWidget::restoreState( QDataStream &stream )
{
    stream >> m_urlsToOpen;
    KonqBaseListViewWidget::restoreState( stream );
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

void KonqTreeViewWidget::slotCompleted( const KUrl & _url )
{
    // do nothing if the view itself is finished
    if ( m_url.equals( _url, KUrl::CompareWithoutTrailingSlash ) )
        return;

    KonqListViewDir *dir = m_dictSubDirs[ _url.url( KUrl::RemoveTrailingSlash ) ];
    if ( dir )
        dir->setComplete( true );
    else
        kWarning() << "KonqTreeViewWidget::slotCompleted : dir " << _url.url( KUrl::RemoveTrailingSlash ) << " not found in dict!" << endl;

    if ( !viewport()->updatesEnabled() )
    {
        viewport()->setUpdatesEnabled( true );
        setUpdatesEnabled( true );
        triggerUpdate();
    }
}

void KonqTreeViewWidget::slotClear()
{
   kDebug(1202) << k_funcinfo << endl;

   m_dictSubDirs.clear();
   KonqBaseListViewWidget::slotClear();
}

void KonqTreeViewWidget::slotClear( const KUrl & _url )
{
   // normally this means we have to delete only the contents of directory _url
   // but we are allowed to delete the subdirs as well since the opening of
   // subdirs happens level per level. If a subdir is deleted with delete, all
   // its children will be deleted by Qt immediately!

   kDebug(1202) << k_funcinfo << _url << endl;

   KonqListViewDir *item = m_dictSubDirs[_url.url( KUrl::RemoveTrailingSlash )];
   if ( item )
   {
      // search all subdirs of _url (item)
      Q3DictIterator<KonqListViewDir> it( m_dictSubDirs );
      while ( it.current() )
      {
         if ( !_url.equals( it.currentKey(), KUrl::CompareWithoutTrailingSlash )
              && _url.isParentOf( it.currentKey() ) )
         {
            m_urlsToOpen.removeAll( it.currentKey() );
            m_urlsToReload.removeAll( it.currentKey() );
            m_dictSubDirs.remove( it.currentKey() );  // do last, it changes it.currentKey()!!
         }
         else
            ++it;
      }

      // Remark: This code works only if we have exactly one tree which is the
      // case for Konqy's treeview. It will break if m_dictSubDirs contains two
      // subdirectories where only one of them will have its items deleted by
      // the following code.

      // delete all child items, their fileitems are no longer valid
      Q3ListViewItem *child;
      while ( (child = item->firstChild()) )
         delete child;

      // only if we really deleted something update the statusbar
      reportItemCounts();
   }
}

void KonqTreeViewWidget::slotRedirection( const KUrl &oldUrl, const KUrl &newUrl )
{
   kDebug(1202) << k_funcinfo << oldUrl.url() << " -> " << newUrl.url() << endl;

   KonqListViewDir *dir = m_dictSubDirs.take( oldUrl.url( KUrl::RemoveTrailingSlash ) );
   Q_ASSERT( dir );
   m_dictSubDirs.insert( newUrl.url( KUrl::RemoveTrailingSlash ), dir );
   // TODO: do we need to rename the fileitem in dir as well?
}

void KonqTreeViewWidget::slotNewItems( const KFileItemList &entries )
{
    // Find parent item - it's the same for all the items
    KUrl dir( entries.first()->url().upUrl() );

    KonqListViewDir *parentDir = 0L;
    if ( !m_url.equals( dir, KUrl::CompareWithoutTrailingSlash ) ) // ignore trailing slash
        parentDir = m_dictSubDirs[ dir.url( KUrl::RemoveTrailingSlash ) ];

    if ( !parentDir )   // hack for zeroconf://domain/type/service listed in zeroconf:/type/ dir
    {
    	dir.setHost( QString() );
	parentDir = m_dictSubDirs[ dir.url( KUrl::RemoveTrailingSlash ) ];
    }

    KFileItemList::const_iterator kit = entries.begin();
    const KFileItemList::const_iterator kend = entries.end();
    for (; kit != kend; ++kit )
    {
        KonqListViewDir *dirItem = 0;
        KonqListViewItem *fileItem = 0;

        if ( parentDir ) // adding under a directory item
        {
            if ( (*kit)->isDir() )
            {
                dirItem = new KonqListViewDir( this, parentDir, *kit );
                m_dictSubDirs.insert( (*kit)->url().url( KUrl::RemoveTrailingSlash ), dirItem );
            }
            else
                fileItem = new KonqListViewItem( this, parentDir, *kit );
        }
        else   // adding on the toplevel
        {
            if ( (*kit)->isDir() )
            {
                dirItem = new KonqListViewDir( this, *kit );
                m_dictSubDirs.insert( (*kit)->url().url( KUrl::RemoveTrailingSlash ), dirItem );
            }
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
                setCurrentItem( dirItem );
                m_itemFound = true;
            }
        }

        if ( !m_itemsToSelect.isEmpty() ) {
           int tsit = m_itemsToSelect.indexOf( (*kit)->name() );
           if ( tsit >= 0 ) {
              m_itemsToSelect.removeAt( tsit );
              setSelected( fileItem ? fileItem : dirItem, true );
           }
        }

        if ( fileItem && !(*kit)->isMimeTypeKnown() )
            m_pBrowserView->lstPendingMimeIconItems().append( fileItem );

        if ( dirItem )
        {
            QString u = (*kit)->url().url( KUrl::LeaveTrailingSlash );
            if ( m_urlsToOpen.removeAll( u ) )
                dirItem->open( true, false );
            else if ( m_urlsToReload.removeAll( u ) )
                dirItem->open( true, true );
        }
    }

    if ( !viewport()->updatesEnabled() )
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
    QString url = _fileItem->url().url( KUrl::RemoveTrailingSlash );

    // Check if this item is in m_dictSubDirs, and if yes, then remove it
    slotClear( _fileItem->url() );

    m_dictSubDirs.remove( url );
    m_urlsToOpen.removeAll( url );
    m_urlsToReload.removeAll( url );

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
