/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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
#include "konq_listviewitems.h"
#include "konq_treeviewwidget.h"

#include <kdebug.h>
#include <konq_dirlister.h>

#include <konq_settings.h>
#include "konq_propsview.h"

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
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
   // Remove all items
   clear();
   // Clear dict
   m_dictSubDirs.clear();
   kdDebug(1202) << "-KonqTreeViewWidget" << endl;
}

bool KonqTreeViewWidget::openURL( const KURL &url )
{
   m_urlsToOpen.clear();
   if ( m_pBrowserView->extension()->urlArgs().reload )
   {
      QDictIterator<KonqListViewDir> it( m_dictSubDirs );
      for (; it.current(); ++it )
         if ( it.current()->isOpen() )
            m_urlsToOpen.append( it.current()->url( -1 ) );
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

void KonqTreeViewWidget::setComplete()
{
   if ( m_itemsToOpen.count() > 0 )
   {
      KonqListViewDir *dir = m_itemsToOpen.take( 0 );
      dir->setOpen( true );
   } else
      KonqBaseListViewWidget::setComplete();

   slotOnViewport();
}

void KonqTreeViewWidget::slotCompleted( const KURL & _url )
{
    if( m_url.cmp( _url, true ) ) // ignore trailing slash
        return;
    KonqListViewDir *dir = m_dictSubDirs[ _url.url(-1) ];
    ASSERT( dir );
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
   kdDebug(1202) << "KonqTreeViewWidget::slotClear()" << endl;

   m_dictSubDirs.clear();
   KonqBaseListViewWidget::slotClear();
}

void KonqTreeViewWidget::slotNewItems( const KFileItemList & entries )
{
    // Find parent item - it's the same for all the items
    QListIterator<KFileItem> kit ( entries );
    KURL dir ( (*kit)->url() );
    dir.setFileName( "" );
    //kdDebug(1202) << "dir = " << dir.url() << endl;
    KonqListViewDir * parentDir = 0L;
    if( !m_url.cmp( dir, true ) ) // ignore trailing slash
    {
        parentDir = m_dictSubDirs[ dir.url(-1) ];
        kdDebug(1202) << "found in the dict: " << parentDir << endl;
    }

    for( ; kit.current(); ++kit )
    {
        KonqListViewDir *dirItem = 0;
        KonqListViewItem *fileItem = 0;

        if ( parentDir )
        { // adding under a directory item
            if ( (*kit)->isDir() )
                dirItem = new KonqListViewDir( this, parentDir, static_cast<KonqFileItem*>(*kit) );
            else
                fileItem = new KonqListViewItem( this, parentDir, static_cast<KonqFileItem*>(*kit) );
        }
        else
        { // adding on the toplevel
            if ( (*kit)->isDir() )
                dirItem = new KonqListViewDir( this, static_cast<KonqFileItem*>(*kit) );
            else
                fileItem = new KonqListViewItem( this, static_cast<KonqFileItem*>(*kit) );
        }

        if (m_goToFirstItem==false)
            if (m_itemFound==false)
            {
                if (fileItem)
                {
                    if (fileItem->text(0)==m_itemToGoTo)
                    {
                        setCurrentItem(fileItem);
                        m_itemFound=true;
                    };
                }
                else if (dirItem)
                {
                    if (dirItem->text(0)==m_itemToGoTo)
                    {
                        setCurrentItem(dirItem);
                        m_itemFound=true;
                    };
                };
                if (m_itemFound)
                {
                    ensureItemVisible(currentItem());
                    emit selectionChanged();
                    //selectCurrentItemAndEnableSelectedBySimpleMoveMode();
                };
            };

        if ( fileItem && !(*kit)->isMimeTypeKnown() )
            m_pBrowserView->lstPendingMimeIconItems().append( fileItem );

        QString u = (*kit)->url().url( 0 );

        if ( dirItem && m_urlsToOpen.contains( u ) )
        {
            m_itemsToOpen.append( dirItem );
            m_urlsToOpen.remove( u );
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
}

void KonqTreeViewWidget::slotDeleteItem( KFileItem *_fileItem )
{
    QString url = _fileItem->url().url( 0 );
    m_urlsToOpen.remove( url );
    QListIterator<KonqListViewDir> it( m_itemsToOpen );
    for (; it.current(); ++it )
        if ( it.current()->url( 0 ) == url )
        {
            m_pBrowserView->lstPendingMimeIconItems().remove( it.current() );
            m_itemsToOpen.removeRef( it.current() );
            break;
        }

    // Check if this item is in m_dictSubDirs, and if yes, then remove it
    removeSubDir( _fileItem->url().url(-1) );

    KonqBaseListViewWidget::slotDeleteItem( _fileItem );
}

void KonqTreeViewWidget::openSubFolder( KonqListViewDir* _dir )
{
   m_dirLister->openURL( _dir->item()->url(), props()->isShowingDotFiles(), true /* keep existing data */ );
}

void KonqTreeViewWidget::stopListingSubFolder( KonqListViewDir* _dir )
{
   m_dirLister->stop( _dir->item()->url() );
}

#include "konq_treeviewwidget.moc"
