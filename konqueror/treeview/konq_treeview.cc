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

#include "konq_treeview.h"
#include "konq_treeviewitems.h"
#include "konq_treeviewwidget.h"
#include "konq_factory.h"

#include <kcursor.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio_paste.h>
#include <kio_job.h>
#include <kdebug.h>
#include <konq_propsview.h>
#include <kuserpaths.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <qkeycode.h>
#include <qlist.h>
#include <qdragobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <klocale.h>
#include <klibloader.h>

class KonqTreeViewFactory : public KLibFactory
{
public:
  KonqTreeViewFactory() 
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqTreeViewFactory()
  {
    KonqFactory::instanceUnref();
  }
  
  virtual QObject* create( QObject*, const char*, const char*, const QStringList & )
  {
    QObject *obj = new KonqTreeView;
    emit objectCreated( obj );
    return obj;
  }
  
};

extern "C"
{
  void *init_libkonqtreeview()
  {
    return new KonqTreeViewFactory;
  }
};

TreeViewPropertiesExtension::TreeViewPropertiesExtension( KonqTreeView *treeView )
  : ViewPropertiesExtension( treeView, "ViewPropertiesExtension" )
{
  m_treeView = treeView;
}

void TreeViewPropertiesExtension::reparseConfiguration()
{
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy : 
  m_treeView->treeViewWidget()->initConfig();
}

void TreeViewPropertiesExtension::saveLocalProperties()
{
  // TODO move this to KonqTreeView. Ugly.
  m_treeView->treeViewWidget()->m_pProps->saveLocal( KURL( m_treeView->url() ) );
}

void TreeViewPropertiesExtension::savePropertiesAsDefault()
{
  m_treeView->treeViewWidget()->m_pProps->saveAsDefault();
}

TreeViewEditExtension::TreeViewEditExtension( KonqTreeView *treeView )
 : EditExtension( treeView, "TreeViewEditExtension" )
{
  m_treeView = treeView;
}

void TreeViewEditExtension::can( bool &cut, bool &copy, bool &paste, bool &move )
{
  QValueList<KonqTreeViewItem*> selection;
  
  m_treeView->treeViewWidget()->selectedItems( selection );
  
  cut = move = copy = ( selection.count() != 0 );
  bool bInTrash = false;
  QValueList<KonqTreeViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqTreeViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
  {
    if ( (*it)->item()->url().directory(false) == KUserPaths::trashPath() )
      bInTrash = true;
  }
  move = move && !bInTrash;
  
  bool bKIOClipboard = !isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (selection.count() == 1); // Let's allow pasting only on an item, not on the background
}

void TreeViewEditExtension::cutSelection()
{
  //TODO: grey out items
  copySelection();
}

void TreeViewEditExtension::copySelection()
{
  QValueList<KonqTreeViewItem*> selection;
  
  m_treeView->treeViewWidget()->selectedItems( selection );
  
  QStringList lstURLs;
  
  QValueList<KonqTreeViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqTreeViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
    lstURLs.append( (*it)->item()->url().url() );
    
  QUriDrag *urlData = new QUriDrag;
  urlData->setUnicodeUris( lstURLs );
  QApplication::clipboard()->setData( urlData );
}

void TreeViewEditExtension::pasteSelection( bool move )
{
  QValueList<KonqTreeViewItem*> selection;
  m_treeView->treeViewWidget()->selectedItems( selection );
  assert ( selection.count() == 1 );
  pasteClipboard( selection.first()->item()->url().url(), move );
}

void TreeViewEditExtension::moveSelection( const QString &destinationURL )
{
  QValueList<KonqTreeViewItem*> selection;
  m_treeView->treeViewWidget()->selectedItems( selection );
  
  QStringList lstURLs;
  
  QValueList<KonqTreeViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqTreeViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
    lstURLs.append( (*it)->item()->url().url() );
    
  KIOJob *job = new KIOJob;
  
  if ( !destinationURL.isEmpty() )
    job->move( lstURLs, destinationURL );
  else
    job->del( lstURLs );
}

KonqTreeView::KonqTreeView()
{
  EditExtension *extension = new TreeViewEditExtension( this );
  (void)new TreeViewPropertiesExtension( this );
  m_pTreeView = new KonqTreeViewWidget( this );
  m_pTreeView->show();

  m_paShowDot = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), this );

  actions()->append( BrowserView::ViewAction( m_paShowDot, BrowserView::MenuView ) );

  QObject::connect( m_pTreeView, SIGNAL( selectionChanged() ),
                    extension, SIGNAL( selectionChanged() ) );
}

KonqTreeView::~KonqTreeView()
{
  delete m_pTreeView;
}

void KonqTreeView::openURL( const QString &url, bool /*reload*/,
                            int xOffset, int yOffset )
{
  m_pTreeView->openURL( url, xOffset, yOffset );
}

QString KonqTreeView::url()
{
  return m_pTreeView->url();
}

int KonqTreeView::xOffset()
{
  return m_pTreeView->contentsX();
}

int KonqTreeView::yOffset()
{
  return m_pTreeView->contentsY();
}

void KonqTreeView::stop()
{
  m_pTreeView->stop();
}

void KonqTreeView::resizeEvent( QResizeEvent * )
{
  m_pTreeView->setGeometry( 0, 0, width(), height() );
}

void KonqTreeView::slotReloadTree()
{
  m_pTreeView->openURL( url(), m_pTreeView->contentsX(), m_pTreeView->contentsY() );
}

void KonqTreeView::slotShowDot()
{
  m_pTreeView->dirLister()->setShowingDotFiles( m_paShowDot->isChecked() );
}

#include "konq_treeview.moc"

