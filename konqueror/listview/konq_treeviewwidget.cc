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

#include <qdragobject.h>
#include <qheader.h>

#include <kdebug.h>
#include <konq_dirlister.h>
#include <kglobal.h>
#include <klocale.h>
#include <stdlib.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>

#include <konq_settings.h>
#include "konq_propsview.h"

#include <assert.h>

template class QDict<KonqListViewDir>;

//////////////////////////////////////////////
KonqTreeViewWidget::KonqTreeViewWidget( KonqListView *parent, QWidget *parentWidget)
: KonqBaseListViewWidget(parent,parentWidget)
,m_pWorkingDir(0L)
,m_bSubFolderComplete(TRUE)
{
   kdDebug(1202) << "+KonqTreeViewWidget" << endl;

   setRootIsDecorated( true );
   setTreeStepSize( 20 );
   setSelectionModeExt( Multi ); // trying
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
   // Remove all items
   clear();
   // Clear dict
   m_dictSubDirs.clear();
   kdDebug(1202) << "-KonqTreeViewWidget" << endl;
}

void KonqTreeViewWidget::saveState( QDataStream &stream )
{
    QStringList openDirList;

    QDictIterator<KonqListViewDir> it( m_dictSubDirs );
    for (; it.current(); ++it )
        if ( it.current()->isOpen() )
            openDirList.append( it.current()->url( -1 ) );

    stream << openDirList;
}

void KonqTreeViewWidget::restoreState( QDataStream &stream )
{
    stream >> m_urlsToOpen;
}

void KonqTreeViewWidget::addSubDir(const KURL & _url, KonqListViewDir* _dir)
{
   m_dictSubDirs.insert( _url.url(-1), _dir );
}

void KonqTreeViewWidget::removeSubDir( const KURL & _url )
{
   m_dictSubDirs.remove( _url.url(-1) );
}

void KonqTreeViewWidget::setComplete()
{
   if ( m_pWorkingDir )
   {
      m_bSubFolderComplete = true;
      m_pWorkingDir->setComplete( true );
      m_pWorkingDir = 0L;
   }
   else
   {
      m_bTopLevelComplete = true;
      KonqBaseListViewWidget::setComplete();
   }

   if ( m_itemsToOpen.count() > 0 )
   {
       KonqListViewDir *dir = m_itemsToOpen.take( 0 );
       dir->setOpen( true );
   }
}

void KonqTreeViewWidget::slotClear()
{
   kdDebug(1202) << "KonqTreeViewWidget::slotClear()" << endl;
   if ( !m_pWorkingDir )
   {
      KonqBaseListViewWidget::slotClear();
      m_dictSubDirs.clear();
   }
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
                fileItem = new KonqListViewItem( this,static_cast<KonqFileItem*> (*kit) );
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

void KonqTreeViewWidget::openSubFolder(const KURL &_url, KonqListViewDir* _dir)
{
   if ( !m_bTopLevelComplete )
   {
      // TODO: Give a warning
      kdDebug(1202) << "Still waiting for toplevel directory" << endl;
      return;
   }

   // If we are opening another sub folder right now, stop it.
   if ( !m_bSubFolderComplete )
   {
      m_dirLister->stop();
   }

   /** Debug code **/
   //assert( m_iColumns != -1 && m_dirLister );
   //if ( m_dirLister->url().protocol() != _url.protocol() )
   //   assert( 0 ); // not same protocol as parent dir...
   /** End Debug code **/

   m_bSubFolderComplete = false;
   m_pWorkingDir = _dir;
   m_dirLister->openURL( _url, props()->isShowingDotFiles(), true /* keep existing data */ );
}

#include "konq_treeviewwidget.moc"
