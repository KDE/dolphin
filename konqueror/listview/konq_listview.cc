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
#include "konq_listviewwidget.h"
#include "konq_factory.h"

#include <kcursor.h>
#include <kdirlister.h>
#include <konqfileitem.h>
#include <kio/paste.h>
#include <kio/job.h>
#include <kdebug.h>
#include <konq_propsview.h>
#include <kaction.h>
#include <kparts/mainwindow.h>
#include <kparts/partmanager.h>
#include <kparts/factory.h>

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
#include <qstringlist.h>
#include <klocale.h>
#include <klibloader.h>

class KonqListViewFactory : public KParts::Factory
{
public:
  KonqListViewFactory()
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqListViewFactory()
  {
    KonqFactory::instanceUnref();
  }

  virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList &args )
  {
    if( args.count() < 1 )
      kdWarning() << "KonqListView: Missing Parameter" << endl;

    KParts::Part *obj = new KonqListView( parentWidget, parent, name, args.first() );
    emit objectCreated( obj );
    return obj;
  }

};

extern "C"
{
  void *init_libkonqlistview()
  {
    return new KonqListViewFactory;
  }
};

ListViewBrowserExtension::ListViewBrowserExtension( KonqListView *listView )
 : KParts::BrowserExtension( listView )
{
  m_listView = listView;
}

int ListViewBrowserExtension::xOffset()
{
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
  return m_listView->listViewWidget()->contentsY();
}

void ListViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged

  QValueList<KonqListViewItem*> selection;
  m_listView->listViewWidget()->selectedItems( selection );

  bool cutcopy, del;
  bool bInTrash = false;
  QValueList<KonqListViewItem*>::ConstIterator it = selection.begin();
  for (; it != selection.end(); ++it )
  {
    if ( (*it)->item()->url().directory(false) == KUserPaths::trashPath() )
      bInTrash = true;
  }
  cutcopy = del = ( selection.count() > 0 );

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "trash", del && !bInTrash );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (selection.count() == 1); // Let's allow pasting only on an item, not on the background

  emit enableAction( "pastecut", paste );
  emit enableAction( "pastecopy", paste );
}

void ListViewBrowserExtension::cut()
{
  //TODO: grey out item
  copy();
}

void ListViewBrowserExtension::copy()
{
  QValueList<KonqListViewItem*> selection;

  m_listView->listViewWidget()->selectedItems( selection );

  QStringList lstURLs;

  QValueList<KonqListViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqListViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
    lstURLs.append( (*it)->item()->url().url() );

  QUriDrag *urlData = new QUriDrag;
  urlData->setUnicodeUris( lstURLs );
  QApplication::clipboard()->setData( urlData );
}

void ListViewBrowserExtension::pasteSelection( bool move )
{
  QValueList<KonqListViewItem*> selection;
  m_listView->listViewWidget()->selectedItems( selection );
  assert ( selection.count() == 1 );
  KIO::pasteClipboard( selection.first()->item()->url(), move );
}

void ListViewBrowserExtension::reparseConfiguration()
{
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy :
  m_listView->listViewWidget()->initConfig();
}

void ListViewBrowserExtension::saveLocalProperties()
{
  // TODO move this to KonqListView. Ugly.
  m_listView->listViewWidget()->m_pProps->saveLocal( m_listView->url() );
}

void ListViewBrowserExtension::savePropertiesAsDefault()
{
  m_listView->listViewWidget()->m_pProps->saveAsDefault();
}

KonqListView::KonqListView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode )
 : KParts::ReadOnlyPart( parent, name )
{
  setInstance( KonqFactory::instance() );
  setXMLFile( "konq_treeview.rc" );

  m_browser = new ListViewBrowserExtension( this );

  m_pListView = new KonqListViewWidget( this, parentWidget, mode );

  setWidget( m_pListView );

  m_paShowDot = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );

  QObject::connect( m_pListView, SIGNAL( selectionChanged() ),
                    m_browser, SLOT( updateActions() ) );
}

KonqListView::~KonqListView()
{
}

bool KonqListView::openURL( const KURL &url )
{
  m_url = url;

  KURL u( url );

  emit setWindowCaption( u.decodedURL() );

  return m_pListView->openURL( url );
}

bool KonqListView::closeURL()
{
  m_pListView->stop();
  return true;
}

void KonqListView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  KParts::ReadOnlyPart::guiActivateEvent( event );
  if ( event->activated() )
    m_browser->updateActions();
}

void KonqListView::slotReloadTree()
{
//  m_pListView->openURL( url(), m_pListView->contentsX(), m_pListView->contentsY() );
}

void KonqListView::slotShowDot()
{
  m_pListView->dirLister()->setShowingDotFiles( m_paShowDot->isChecked() );
}

#include "konq_listview.moc"


