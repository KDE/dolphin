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
#include <kio/paste.h>
#include <kio/job.h>
#include <kdebug.h>
#include <konq_propsview.h>
#include <kaction.h>
#include <kparts/mainwindow.h>
#include <kparts/partmanager.h>

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

  virtual QObject* create( QObject *parent, const char *name, const char*, const QStringList & )
  {
    QObject *obj = new KonqTreeView( (QWidget *)parent, parent, name );
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

TreeViewBrowserExtension::TreeViewBrowserExtension( KonqTreeView *treeView )
 : KParts::BrowserExtension( treeView )
{
  m_treeView = treeView;
}

void TreeViewBrowserExtension::setXYOffset( int x, int y )
{
  m_treeView->treeViewWidget()->setXYOffset( x, y );
}

int TreeViewBrowserExtension::xOffset()
{
  return m_treeView->treeViewWidget()->contentsX();
}

int TreeViewBrowserExtension::yOffset()
{
  return m_treeView->treeViewWidget()->contentsY();
}

void TreeViewBrowserExtension::updateActions()
{
  QValueList<KonqTreeViewItem*> selection;

  m_treeView->treeViewWidget()->selectedItems( selection );

  bool cutcopy, move;

  cutcopy = move = ( selection.count() != 0 );
  bool bInTrash = false;
  QValueList<KonqTreeViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqTreeViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
  {
    if ( (*it)->item()->url().directory(false) == KUserPaths::trashPath() )
      bInTrash = true;
  }
  move = move && !bInTrash;

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (selection.count() == 1); // Let's allow pasting only on an item, not on the background

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "del", move );
  emit enableAction( "trash", move );
  emit enableAction( "pastecut", paste );
  emit enableAction( "pastecopy", paste );
}

void TreeViewBrowserExtension::cut()
{
  //TODO: grey out item
  copy();
}

void TreeViewBrowserExtension::copy()
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

void TreeViewBrowserExtension::pasteSelection( bool move )
{
  QValueList<KonqTreeViewItem*> selection;
  m_treeView->treeViewWidget()->selectedItems( selection );
  assert ( selection.count() == 1 );
  KIO::pasteClipboard( selection.first()->item()->url(), move );
}

void TreeViewBrowserExtension::reparseConfiguration()
{
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy :
  m_treeView->treeViewWidget()->initConfig();
}

void TreeViewBrowserExtension::saveLocalProperties()
{
  // TODO move this to KonqTreeView. Ugly.
  m_treeView->treeViewWidget()->m_pProps->saveLocal( m_treeView->url() );
}

void TreeViewBrowserExtension::savePropertiesAsDefault()
{
  m_treeView->treeViewWidget()->m_pProps->saveAsDefault();
}

KonqTreeView::KonqTreeView( QWidget *parentWidget, QObject *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
  setInstance( KonqFactory::instance() );
  setXMLFile( "konq_treeview.rc" );

  m_browser = new TreeViewBrowserExtension( this );

  m_pTreeView = new KonqTreeViewWidget( this, parentWidget );

  setWidget( m_pTreeView );

  m_paShowDot = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );

  QObject::connect( m_pTreeView, SIGNAL( selectionChanged() ),
                    m_browser, SLOT( updateActions() ) );
}

KonqTreeView::~KonqTreeView()
{
}

bool KonqTreeView::openURL( const KURL &url )
{
  m_url = url;

  KURL u( url );

  emit setWindowCaption( u.decodedURL() );

  return m_pTreeView->openURL( url );
}

bool KonqTreeView::closeURL()
{
  m_pTreeView->stop();
  return true;
}

void KonqTreeView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  KParts::ReadOnlyPart::guiActivateEvent( event );
  if ( event->activated() )
    m_browser->updateActions();
}

void KonqTreeView::slotReloadTree()
{
//  m_pTreeView->openURL( url(), m_pTreeView->contentsX(), m_pTreeView->contentsY() );
}

void KonqTreeView::slotShowDot()
{
  m_pTreeView->dirLister()->setShowingDotFiles( m_paShowDot->isChecked() );
}

#include "konq_treeview.moc"


