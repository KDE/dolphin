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
#include <konqdirlister.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <klocale.h>
#include <stdlib.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>

#include <konqsettings.h>
#include "konq_propsview.h"

#include <assert.h>

template class QDict<KonqListViewDir>;

//////////////////////////////////////////////
KonqTreeViewWidget::KonqTreeViewWidget( KonqListView *parent, QWidget *parentWidget)
: KonqBaseListViewWidget(parent,parentWidget)
,m_pWorkingDir(0L)
,m_lasttvd(0L)
,m_bSubFolderComplete(TRUE)
{
   kdDebug(1202) << "+KonqTreeViewWidget" << endl;

   setRootIsDecorated( true );
   setTreeStepSize( 20 );
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
   // Remove all items - for dirs, this calls removeSubDir
   clear();
   kdDebug(1202) << "-KonqTreeViewWidget" << endl;
}

void KonqTreeViewWidget::addSubDir(const KURL & _url, KonqListViewDir* _dir)
{
   m_mapSubDirs.insert( _url.url(), _dir );
   if ( _url.isLocalFile() )
      kdirwatch->addDir( _url.path(0) );
}

void KonqTreeViewWidget::removeSubDir( const KURL & _url )
{
   m_lasttvd = 0L; // drop cache, to avoid segfaults
   m_mapSubDirs.remove( _url.url() );

   if ( _url.isLocalFile() )
      kdirwatch->removeDir( _url.path(0) );
}

void KonqTreeViewWidget::slotReturnPressed( QListViewItem *_item )
{
   if ( !_item )
      return;
   KonqFileItem *item = ((KonqBaseListViewItem*)_item)->item();
   mode_t mode = item->mode();

   //execute only if item is a file (or a symlink to a file)
   if ( S_ISREG( mode ) )
   {
      KParts::URLArgs args;
      args.serviceType = item->mimetype();
      emitOpenURLRequest(item->url(), args);
   }
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
      setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset, m_pBrowserView->extension()->urlArgs().yOffset );
   }
}

void KonqTreeViewWidget::slotClear()
{
   kdDebug(1202) << "KonqTreeViewWidget::slotClear()" << endl;
   if ( !m_pWorkingDir )
      clear();
}

void KonqTreeViewWidget::slotNewItems( const KFileItemList & entries )
{
   QListIterator<KFileItem> kit ( entries );
   for( ; kit.current(); ++kit )
   {
      bool isdir = S_ISDIR( (*kit)->mode() );

      KURL dir ( (*kit)->url() );
      dir.setFileName( "" );
      //kdDebug(1202) << "dir = " << dir.url() << endl;
      KonqListViewDir * parentDir = 0L;
      if( !m_url.cmp( dir, true ) ) // ignore trailing slash
      {
         parentDir = findDir ( dir.url( 0 ) );
         kdDebug(1202) << "findDir returned " << parentDir << endl;
      }

      if ( parentDir )
      { // adding under a directory item
         if ( isdir )
            new KonqListViewDir( this, parentDir, static_cast<KonqFileItem*>(*kit) );
         else
            new KonqListViewItem( this, parentDir, static_cast<KonqFileItem*>(*kit) );
      }
      else
      { // adding on the toplevel
         if ( isdir )
            new KonqListViewDir( this, static_cast<KonqFileItem*>(*kit) );
         else
            new KonqListViewItem( this,static_cast<KonqFileItem*> (*kit) );
      }
   }
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
   if ( strcmp( m_dirLister->url().protocol(), _url.protocol() ) != 0 )
      assert( 0 ); // not same protocol as parent dir -> abort
   /** End Debug code **/

   m_bSubFolderComplete = false;
   m_pWorkingDir = _dir;
   m_dirLister->openURL( _url, props()->isShowingDotFiles(), true /* keep existing data */ );
}

KonqListViewDir * KonqTreeViewWidget::findDir( const QString &_url )
{
   if ( m_lasttvd && urlcmp( m_lasttvd->url(0), _url, true, true ) )
      return m_lasttvd;

   QDictIterator<KonqListViewDir> it( m_mapSubDirs );
   for( ; it.current(); ++it )
   {
      kdDebug(1202) << it.current()->url(0) << endl;
      if ( urlcmp( it.current()->url(0), _url, true, true ) )
      {
         m_lasttvd = it.current();
         return it.current();
      }
   }
   return 0L;
}

#include "konq_treeviewwidget.moc"
