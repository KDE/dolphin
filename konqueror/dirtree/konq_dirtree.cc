/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_dirtree.h"

#include <qdir.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qcursor.h>
#include <qdragobject.h>
#include <qfileinfo.h>
#include <qheader.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kdesktopfile.h>
#include <konq_dirlister.h>
#include <kdirnotify.h>
#include <kdirnotify_stub.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <klibloader.h>
#include <klocale.h>
#include <kurldrag.h>
#include <konq_drag.h>
#include <konq_fileitem.h>
#include <konq_operations.h>
#include <konq_settings.h>
#include <kparts/factory.h>
#include <kprotocolinfo.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

#include <assert.h>

inline bool operator<( const KURL &u1, const KURL &u2 )
{
  return u1.url( 0 ) < u2.url( 0 );
}

class KonqDirTreeFactory : public KParts::Factory
{
public:
  KonqDirTreeFactory()
  {
  }
  virtual ~KonqDirTreeFactory()
  {
    if ( s_instance )
    {
      delete s_instance;
      s_instance = 0L;
    }
  }

  virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList & )
  {
    KParts::Part *obj = new KonqDirTreePart( parentWidget, parent, name );
    emit objectCreated( obj );
    return obj;
  }

  static KInstance *instance()
  {
    if ( !s_instance )
      s_instance = new KInstance( "konqueror" );
    return s_instance;
  }

private:
  static KInstance *s_instance;
};

KInstance *KonqDirTreeFactory::s_instance = 0;

extern "C"
{
  void *init_libkonqdirtree()
  {
    return new KonqDirTreeFactory;
  }
};

KonqDirTreeBrowserExtension::KonqDirTreeBrowserExtension( KonqDirTreePart *parent, KonqDirTree *dirTree )
 : KParts::BrowserExtension( parent )
{
  m_tree = dirTree;
  connect( m_tree, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
}

void KonqDirTreeBrowserExtension::slotSelectionChanged()
{
  // This code is very related to TreeViewBrowserExtension::updateActions
  // (but simpler this here we can have only one selected item)
  bool cutcopy, del;
  bool bInTrash = false;

  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_tree->selectedItem() );

  if ( selection && selection->fileItem()->url().directory(false) == KGlobalSettings::trashPath() )
    bInTrash = true;

  cutcopy = del = selection;

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "trash", del && !bInTrash );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( data->encodedData( data->format() ).size() != 0 ) && selection;

  emit enableAction( "paste", paste );
}

void KonqDirTreeBrowserExtension::cut()
{
  KonqDrag * drag = konqDragObject();
  if (drag)
  {
    drag->setMoveSelection( true );
    QApplication::clipboard()->setData( drag );
  }
}

void KonqDirTreeBrowserExtension::copy()
{
  KonqDrag * drag = konqDragObject();
  if (drag)
    QApplication::clipboard()->setData( drag );
}

KonqDrag * KonqDirTreeBrowserExtension::konqDragObject()
{
  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_tree->selectedItem() );

  if ( !selection )
    return 0L;

  KURL::List lst;
  lst.append( selection->fileItem()->url() );

  return KonqDrag::newDrag( lst, false, 0L /* no parent ! */ );
}

void KonqDirTreeBrowserExtension::paste()
{
  // move or not move ?
  bool move = false;
  QMimeSource *data = QApplication::clipboard()->data();
  if ( data->provides( "application/x-kde-cutselection" ) ) {
    move = KonqDrag::decodeIsCutSelection( data );
    kdDebug(1202) << "move (from clipboard data) = " << move << endl;
  }

  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_tree->selectedItem() );
  assert( selection );
  KIO::pasteClipboard( selection->fileItem()->url(), move );
}

KURL::List KonqDirTreeBrowserExtension::selectedUrls()
{
  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_tree->selectedItem() );
  assert( selection );
  KURL::List lst;
  lst.append(selection->fileItem()->url());
  return lst;
}

KonqDirTreePart::KonqDirTreePart( QWidget *parentWidget, QObject *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
  m_pTree = new KonqDirTree( this, parentWidget );

  m_extension = new KonqDirTreeBrowserExtension( this, m_pTree );

  setWidget( m_pTree );
  setInstance( KonqDirTreeFactory::instance(), false );
  m_url.setPath( QDir::homeDirPath() );
}

KonqDirTreePart::~KonqDirTreePart()
{
}

bool KonqDirTreePart::openURL( const KURL & url )
{
  // We need to update m_url so that view-follows-view works
  // (we need other views to follow us, so we need a proper m_url)
  m_url = url;

  emit started( 0 );
  m_pTree->followURL( url );
  emit completed();
  return true;
}

bool KonqDirTreePart::closeURL()
{
  // Nothing to do
  return true;
}

KonqDirTreeItem::KonqDirTreeItem( KonqDirTree *parent, KonqDirTreeItem *parentItem, KonqDirTreeItem *topLevelItem, KonqFileItem *item )
    : QListViewItem( parentItem )
{
  initItem( parent, topLevelItem, item );
}

KonqDirTreeItem::KonqDirTreeItem( KonqDirTree *parent, KonqDirTreeItem *topLevelItem, KonqFileItem *item )
    : QListViewItem( parent )
{
    initItem( parent, topLevelItem, item );
}

void KonqDirTreeItem::initItem( KonqDirTree *parent, KonqDirTreeItem *topLevelItem, KonqFileItem *item )
{
  m_item = item;
  m_topLevelItem = topLevelItem;
  m_tree = parent;
  m_bListable = true;
  m_bClickable = true;
  m_bLink = false;

  if ( m_topLevelItem )
    m_tree->addSubDir( this, m_topLevelItem, m_item->url() );

  setExpandable( true );
}

KonqDirTreeItem::~KonqDirTreeItem()
{
  if ( m_topLevelItem )
    m_tree->removeSubDir( this, m_topLevelItem, m_item->url() );
}

void KonqDirTreeItem::setOpen( bool open )
{
  if ( open & !childCount() && m_bListable )
      m_tree->openSubFolder( this, m_topLevelItem );

  QListViewItem::setOpen( open );
}

void KonqDirTreeItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
    if (m_item->isLink())
    {
        QFont f( _painter->font() );
        f.setItalic( TRUE );
        _painter->setFont( f );
    }
    QListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}

void KonqDirTreeItem::setListable( bool b )
{
  m_bListable = b;
}

KURL KonqDirTreeItem::externalURL() const
{
    if ( isLink() )
    {
        KSimpleConfig config( m_item->url().path() );
        config.setDesktopGroup();
        config.setDollarExpansion(true);
        if ( config.readEntry("Type") == "FSDevice" )
        {
            KURL url;
            url.setPath( config.readEntry("MountPoint") );
            return url;
        }
        else
        {
            KURL url = config.readEntry("URL");
            return url;
        }
    }
    else
        return m_item->url();
}

//TODO: make it configurable via viewprops
static const int autoOpenTimeout = 750;

KonqDirTree::KonqDirTree( KonqDirTreePart *parent, QWidget *parentWidget )
  : KListView( parentWidget )
{

  m_folderPixmap = KMimeType::mimeType( "inode/directory" )->pixmap( KIcon::Desktop, KIcon::SizeSmall );

  setAcceptDrops( true );
  viewport()->setAcceptDrops( true );

  setSelectionMode( QListView::Single );

  m_view = parent;

  m_animationCounter = 1;
  m_animationTimer = new QTimer( this );

  connect( m_animationTimer, SIGNAL( timeout() ),
           this, SLOT( slotAnimation() ) );

  m_dropItem = 0;

  addColumn( "" );
  header()->hide();

  m_autoOpenTimer = new QTimer( this );
  connect( m_autoOpenTimer, SIGNAL( timeout() ),
           this, SLOT( slotAutoOpenFolder() ) );

  connect( this, SIGNAL( doubleClicked( QListViewItem * ) ),
           this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
           this, SLOT( slotRightButtonPressed( QListViewItem * ) ) );
  connect( this, SIGNAL( clicked( QListViewItem * ) ),
           this, SLOT( slotClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( returnPressed( QListViewItem * ) ),
           this, SLOT( slotDoubleClicked( QListViewItem * ) ) );

  m_bDrag = false;

  assert( KonqDirTreeFactory::instance()->dirs() );
  QString dirtreeDir = KonqDirTreeFactory::instance()->dirs()->saveLocation( "data", "konqueror/dirtree/" );
  m_dirtreeDir.setPath( dirtreeDir );

  // Initial parsing
  rescanConfiguration();
}

KonqDirTree::~KonqDirTree()
{
  clearTree();
}

void KonqDirTree::clearTree()
{
  QValueList<TopLevelItem>::Iterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::Iterator end = m_topLevelItems.end();
  for (; it != end; ++it )
  {
    delete (*it).m_item;
    delete (*it).m_dirLister;
    delete (*it).m_mapSubDirs;
    delete (*it).m_lstPendingURLs;
  }
  //  QMap<QListViewItem *,QString>::ConstIterator groupItem = m_groupItems.begin();
  //  for (; groupItem != m_groupItems.end(); ++groupItem )
  //    delete groupItem.key();
  m_topLevelItems.clear();
  m_groupItems.clear();
  m_mapCurrentOpeningFolders.clear();
  clear();
  setRootIsDecorated( true );
}

void KonqDirTree::openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel )
{
  TopLevelItem & topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  KURL u = item->externalURL();

  kdDebug(1202) << "openSubFolder( " << u.url() << " )  topLevel=" << topLevel << endl;

  if ( topLevelItem.m_dirLister->job() == 0 )
  {
    topLevelItem.m_currentlyListedURL = u;
    kdDebug() << "KonqDirTree::openSubFolder m_currentlyListedURL=" << topLevelItem.m_currentlyListedURL.prettyURL() << endl;
    kdDebug() << "KonqDirTree::openSubFolder toplevelitem=" << &topLevelItem << endl;
    kdDebug() << "KonqDirTree::openSubFolder toplevelitem=" << topLevelItem.m_item->externalURL().prettyURL() << endl;
    kdDebug() << "KonqDirTree::openSubFolder dirlister=" << topLevelItem.m_dirLister << endl;
    topLevelItem.m_dirLister->openURL( u, false, topLevel ? true : false );
  }
  else  if ( !topLevelItem.m_lstPendingURLs->contains( u ) )
    topLevelItem.m_lstPendingURLs->append( u );

  if ( !topLevel ) // No animation for toplevel items (breaks the icon)
    return;

  kdDebug(1202) << "m_mapCurrentOpeningFolders.insert( " << u.prettyURL() << " )" << endl;
  m_mapCurrentOpeningFolders.insert( u, item );

  if ( !m_animationTimer->isActive() )
    m_animationTimer->start( 50 );
}

void KonqDirTree::addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url )
{
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  topLevelItem.m_mapSubDirs->insert( url, item );
}

void KonqDirTree::removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const KURL &url )
{
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  topLevelItem.m_mapSubDirs->remove( url );
}

void KonqDirTree::followURL( const KURL &_url )
{
  // Maybe we're there already ?
  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( selectedItem() );
  if (selection && selection->externalURL().cmp( _url, true ))
  {
      ensureItemVisible( selection );
      return;
  }

  kdDebug(1202) << "KonqDirTree::followURL: " << _url.url() << endl;
  KURL uParent( _url.upURL() );

  QValueList<TopLevelItem>::ConstIterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::ConstIterator end = m_topLevelItems.end();
  for (; it != end; ++it )
  {
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = (*it).m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = (*it).m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
    {
      // That's the URL we want to follow -> ensure visible, select, return.
      if ( _url.cmp( dirIt.key(), true ) )
      {
        ensureItemVisible( dirIt.data() );
        setSelected( dirIt.data(), true );
        return;
      }
      // That's the parent directory. Open if not open...
      if ( uParent.cmp( dirIt.key(), true ) )
      {
        if ( !dirIt.data()->isOpen() )
        {
            dirIt.data()->setOpen( true );
            if ( dirIt.data()->childCount() )
            {
                // Immediate opening, if the dir was already listed
                followURL( _url );
                return;
            } else
            {
                m_selectAfterOpening = _url;
                kdDebug(1202) << "KonqDirTree::followURL: m_selectAfterOpening=" << m_selectAfterOpening.url() << endl;
                return; // We know we won't find it
            }
        }
      }
    }
  }
  kdDebug(1202) << "KonqDirTree::followURL: Not found" << endl;
}

void KonqDirTree::contentsDragEnterEvent( QDragEnterEvent * )
{
  m_dropItem = 0;
}

void KonqDirTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
  QListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

  if ( !item || !item->isSelectable() || !static_cast<KonqDirTreeItem*>(item)->fileItem()->acceptsDrops())
  {
    m_dropItem = 0;
    m_autoOpenTimer->stop();
    e->ignore();
    return;
  }

  e->acceptAction();

  setSelected( item, true );

  if ( item != m_dropItem )
  {
    m_autoOpenTimer->stop();
    m_dropItem = item;
    m_autoOpenTimer->start( autoOpenTimeout );
  }
}

void KonqDirTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
  m_dropItem = 0;
}

void KonqDirTree::contentsDropEvent( QDropEvent *ev )
{
  m_autoOpenTimer->stop();

  KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( selectedItem() );

  assert( selection );

  KFileItem * fileItem = selection->fileItem();

  if ( !selection->isClickable() )
  {
      // When dropping something to "Network" or its subdirs, we want to create
      // a desktop link, not to move/copy/link
      KURL url = fileItem->url();
      if ( url.isLocalFile() )
      {
          KURL::List lst;
          if ( KURLDrag::decode( ev, lst ) ) // Are they urls ?
          {
              KURL::List::Iterator it = lst.begin();
              for ( ; it != lst.end() ; it++ )
              {
                  const KURL & targetURL = (*it);
                  KURL linkURL(url);
                  linkURL.addPath( KIO::encodeFileName( targetURL.fileName() )+".desktop" );
                  KSimpleConfig config( linkURL.path() );
                  config.setDesktopGroup();
                  // Don't write a Name field in the desktop file, it makes renaming impossible
                  config.writeEntry( "URL", targetURL.url() );
                  config.writeEntry( "Type", "Link" );
                  QString icon = KMimeType::findByURL( targetURL )->icon( targetURL, false );
                  static const QString& unknown = KGlobal::staticQString("unknown");
                  if ( icon == unknown )
                      icon = KProtocolInfo::icon( targetURL.protocol() );
                  config.writeEntry( "Icon", icon );
                  config.sync();
                  KDirNotify_stub allDirNotify( "*", "KDirNotify*" );
                  linkURL.setPath( linkURL.directory() );
                  allDirNotify.FilesAdded( linkURL );
              }
          } else
              kdError(1202) << "Not a URL !?!" << endl;
      } else
          kdError(1202) << "Dropped to a remote item !  " << url.prettyURL() << endl;
  }
  else
  {
      if (selection->isLink())
           fileItem = new KFileItem( -1, -1, selection->externalURL() );

      KonqOperations::doDrop( fileItem, ev, this );

      if (selection->isLink())
           delete fileItem;
  }
}

void KonqDirTree::contentsMousePressEvent( QMouseEvent *e )
{
  KListView::contentsMousePressEvent( e );

  QPoint p( contentsToViewport( e->pos() ) );
  QListViewItem *i = itemAt( p );

  if ( e->button() == LeftButton && i ) {
      // if the user clicked into the root decoration of the item, don't try to start a drag!
      if ( p.x() > header()->cellPos( header()->mapToActual( 0 ) ) +
           treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() ||
           p.x() < header()->cellPos( header()->mapToActual( 0 ) ) )
      {
        m_dragPos = e->pos();
        m_bDrag = true;
      }
  }
}

void KonqDirTree::contentsMouseMoveEvent( QMouseEvent *e )
{
  if ( !m_bDrag || ( e->pos() - m_dragPos ).manhattanLength() <= KGlobalSettings::dndEventDelay() )
    return;

  m_bDrag = false;

  QListViewItem *item = itemAt( contentsToViewport( m_dragPos ) );

  if ( !item || !item->isSelectable() )
    return;

  KURL::List lst;
  lst.append(static_cast<KonqDirTreeItem *>( item )->fileItem()->url());

  QUriDrag * drag = KURLDrag::newDrag( lst, viewport() );
  QPoint hotspot;
  hotspot.setX( item->pixmap( 0 )->width() / 2 );
  hotspot.setY( item->pixmap( 0 )->height() / 2 );
  drag->setPixmap( *(item->pixmap( 0 )), hotspot );
  drag->drag();

}

void KonqDirTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
  KListView::contentsMouseReleaseEvent( e );
  m_bDrag = false;
}

void KonqDirTree::slotNewItems( const KFileItemList& entries )
{
  kdDebug(1202) << "KonqDirTree::slotNewItems " << entries.count() << endl;

  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem & topLevelItem = findTopLevelByDirLister( lister );
  kdDebug() << "KonqDirTree::slotNewItems topLevelItem=" << &topLevelItem << endl;

  assert( topLevelItem.m_item );

  QListIterator<KFileItem> kit ( entries );
  for( ; kit.current(); ++kit )
  {
    KonqFileItem * item = static_cast<KonqFileItem *>(*kit);

    assert( S_ISDIR( item->mode() ) );

    KURL dir( item->url() );
    dir.setFileName( "" );
    //KURL dir( item->url().directory() );

    //  KonqDirTreeItem *parentDir = findDir( *topLevelItem.m_mapSubDirs, dir.url( 0 ) );
    //  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->find( dir );
    // *mumble* can't use QMap::find() because the cmp doesn't ignore the trailing slash :-(
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = topLevelItem.m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
    {
      if ( dir.cmp( dirIt.key(), true ) )
        break;
    }

    if( dirIt == topLevelItem.m_mapSubDirs->end() )
    {
      // ### TODO Make this a message box
      kdError(1202) << "THIS SHOULD NOT HAPPEN. INTERNAL ERROR" << endl;
      kdError(1202) << "KonqDirTree:slotNewItems got item " << item->url().url() << endl;
      kdError(1202) << "Couldn't find directory " << dir.url() << " in dirtree's m_mapSubDirs" << endl;
      ASSERT( 0 );
      return;
    }

    KonqDirTreeItem *parentDir = dirIt.data();

    assert( parentDir );

    KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( this, parentDir, topLevelItem.m_item, item );
    dirTreeItem->setPixmap( 0, m_folderPixmap );
    dirTreeItem->setText( 0, KIO::decodeFileName( item->url().fileName() ) );
  }
}

void KonqDirTree::slotDeleteItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  //  kdDebug(1202) << "slotDeleteItem( " << item->url().url() << " )" << endl;

  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  if ( item == topLevelItem.m_item->fileItem() )
  {
    assert( 0 ); //TODO ;)
  }

  QListViewItemIterator it( topLevelItem.m_item );
  for (; it.current(); ++it )
  {
    if ( static_cast<KonqDirTreeItem *>( it.current() )->fileItem() == item )
    {
    //      kdDebug(1202) << "removing " << item->url().url() << endl;
      delete it.current();
      return;
    }
  }

}

void KonqDirTree::slotDoubleClicked( QListViewItem *item )
{
  if ( !item )
    return;

  if ( !static_cast<KonqDirTreeItem*>(item)->isClickable() )
    return;

  if ( m_groupItems.find( item ) != m_groupItems.end() )
    return;

  slotClicked( item );
  item->setOpen( !item->isOpen() );
}

void KonqDirTree::slotClicked( QListViewItem *item )
{
  if ( !item )
    return;

  if ( !static_cast<KonqDirTreeItem*>(item)->isClickable() )
    return;

  KonqDirTreeItem *dItem = static_cast<KonqDirTreeItem *>( item );

  KParts::URLArgs args;

  if ( dItem->isListable() )
    args.serviceType = QString::fromLatin1( "inode/directory" );

  args.trustedSource = true;
  emit m_view->extension()->openURLRequest( dItem->externalURL(), args );
}

void KonqDirTree::slotRightButtonPressed( QListViewItem *item )
{
  if ( !item )
    return;

  //if ( !item.isClickable() )
  //  return;

  item->setSelected( true );

  KFileItemList lstItems;

  lstItems.append( (static_cast<KonqDirTreeItem *>(item))->fileItem() );

  kdDebug() << "KonqDirTree::slotRightButtonPressed Emitting popup for " <<  static_cast<KonqDirTreeItem *>(item)->fileItem()->url().prettyURL() << endl;

  emit m_view->extension()->popupMenu( QCursor::pos(), lstItems );
}

void KonqDirTree::slotRedirection( const KURL & url )
{
  kdDebug(1202) << "KonqDirTree::slotRedirection(" << url.prettyURL() << ")" << endl;
  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem & topLevelItem = findTopLevelByDirLister( lister );

  KURL dir = topLevelItem.m_currentlyListedURL;
  QMap<KURL, KonqDirTreeItem *>::Iterator dirIt = topLevelItem.m_mapSubDirs->begin();
  QMap<KURL, KonqDirTreeItem *>::Iterator dirEnd = topLevelItem.m_mapSubDirs->end();
  for (; dirIt != dirEnd; ++dirIt )
  {
    if ( dir.cmp( dirIt.key(), true ) )
      break;
  }

  if (dirIt==dirEnd)
    kdWarning() << "NOT FOUND   dir=" << dir.prettyURL() << endl;
  else
  {
    // We need to update the URL in m_mapSubDirs
    KonqDirTreeItem * item = dirIt.data();
    topLevelItem.m_mapSubDirs->remove( dirIt );
    topLevelItem.m_mapSubDirs->insert( url, item );
    kdDebug() << "Updating url to " << url.prettyURL() << endl;
  }
}

void KonqDirTree::slotListingStopped()
{
  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  KURL url = lister->url();

  kdDebug() << "KonqDirTree::slotListingStopped " << url.prettyURL() << endl;

  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->begin();
  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = topLevelItem.m_mapSubDirs->end();
  for (; dirIt != dirEnd; ++dirIt )
  {
    if ( url.cmp( dirIt.key(), true ) )
      break;
  }

  if ( dirIt != topLevelItem.m_mapSubDirs->end() && dirIt.data()->childCount() == 0 )
  {
      dirIt.data()->setExpandable( false );
      dirIt.data()->repaint();
  }

  KURL::List::Iterator it = topLevelItem.m_lstPendingURLs->find( url );
  if ( it != topLevelItem.m_lstPendingURLs->end() )
  {
    topLevelItem.m_lstPendingURLs->remove( it );
  }

  if ( topLevelItem.m_lstPendingURLs->count() > 0 )
  {
    kdDebug(1202) << "openURL (was pending) " << topLevelItem.m_lstPendingURLs->first().prettyURL() << endl;
    topLevelItem.m_currentlyListedURL = topLevelItem.m_lstPendingURLs->first();
    topLevelItem.m_dirLister->openURL( topLevelItem.m_lstPendingURLs->first(), false, true );
  }

  kdDebug(1202) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
  if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL() == url )
  {
    kdDebug(1202) << "Selecting m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    followURL( m_selectAfterOpening );
    m_selectAfterOpening = KURL();
  }

  QMap<KURL, QListViewItem *>::Iterator oIt = m_mapCurrentOpeningFolders.find( url );
  if ( oIt != m_mapCurrentOpeningFolders.end() )
  {
    oIt.data()->setPixmap( 0, m_folderPixmap );

    m_mapCurrentOpeningFolders.remove( oIt );

    if ( m_mapCurrentOpeningFolders.count() == 0 )
      m_animationTimer->stop();
  }// else kdDebug(1202) << url.prettyURL() << "not found in m_mapCurrentOpeningFolders" << endl;
}

void KonqDirTree::slotAnimation()
{
  QPixmap gearPixmap = SmallIcon( QString::fromLatin1( "kde" ).append( QString::number( m_animationCounter ) ), KonqDirTreeFactory::instance() );

  QMap<KURL, QListViewItem *>::ConstIterator it = m_mapCurrentOpeningFolders.begin();
  QMap<KURL, QListViewItem *>::ConstIterator end = m_mapCurrentOpeningFolders.end();
  for (; it != end; ++it )
    it.data()->setPixmap( 0, gearPixmap );

  m_animationCounter++;
  if ( m_animationCounter == 7 )
    m_animationCounter = 1;
}

void KonqDirTree::slotAutoOpenFolder()
{
  m_autoOpenTimer->stop();

  if ( !m_dropItem || m_dropItem->isOpen() )
    return;

  m_dropItem->setOpen( true );
  m_dropItem->repaint();
}

void KonqDirTree::rescanConfiguration()
{
  kdDebug() << "KonqDirTree::rescanConfiguration()" << endl;
  m_autoOpenTimer->stop();
  clearTree();
  scanDir( 0, m_dirtreeDir.path(), true);
}

void KonqDirTree::FilesAdded( const KURL & dir )
{
  kdDebug(1202) << "KonqDirTree::FilesAdded " << dir.url() << endl;
  if ( m_dirtreeDir.isParentOf( dir ) )
    // We use a timer in case of DCOP re-entrance..
    QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
}

void KonqDirTree::FilesRemoved( const KURL::List & urls )
{
  //kdDebug(1202) << "KonqDirTree::FilesRemoved " << urls.count() << endl;
  for ( KURL::List::ConstIterator it = urls.begin() ; it != urls.end() ; ++it )
  {
    //kdDebug(1202) <<  "KonqDirTree::FilesRemoved " << (*it).prettyURL() << endl;
    if ( m_dirtreeDir.isParentOf( *it ) )
    {
      QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
      kdDebug(1202) << "KonqDirTree::FilesRemoved done" << endl;
      return;
    }
  }
}

void KonqDirTree::FilesChanged( const KURL::List & urls )
{
  //kdDebug(1202) << "KonqDirTree::FilesChanged" << endl;
  // not same signal, but same implementation
  FilesRemoved( urls );
}

void KonqDirTree::scanDir( KonqDirTreeItem *parent, const QString &path, bool isRoot )
{
  QDir dir( path );

  if ( !dir.isReadable() )
    return;

  kdDebug(1202) << "scanDir " << path << endl;

  QStringList entries = dir.entryList( QDir::Files );
  QStringList dirEntries = dir.entryList( QDir::Dirs );
  dirEntries.remove( "." );
  dirEntries.remove( ".." );

  if ( isRoot && entries.count() == 0 && dirEntries.count() == 0 )
  {
    // we will copy over the entire contents of the dirtree directory.
    // to do this, we assume that home.desktop exists..
    QString dirtree_dir = KonqDirTreeFactory::instance()->dirs()->findResourceDir( "data", "konqueror/dirtree/home.desktop" );

    if ( !dirtree_dir.isEmpty() )
    {
      QCString cp;
      cp.sprintf( "cp -R %skonqueror/dirtree/* %s", dirtree_dir.local8Bit().data(),
                                                    path.local8Bit().data() );
      system( cp.data() );
    }
    // hack to make QDir refresh the lists
    dir.setPath(path);
    entries = dir.entryList( QDir::Files );
    dirEntries = dir.entryList( QDir::Dirs );
    dirEntries.remove( "." );
    dirEntries.remove( ".." );
  }

  QStringList::ConstIterator eIt = entries.begin();
  QStringList::ConstIterator eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
  {
    QString filePath = QString( *eIt ).prepend( path );
    KURL u;
    u.setPath( filePath );
    if ( KMimeType::findByURL( u, 0, true )->name() == "application/x-desktop" )
      loadTopLevelItem( parent, filePath );
  }

  eIt = dirEntries.begin();
  eEnd = dirEntries.end();

  for (; eIt != eEnd; eIt++ )
  {
    QString newPath = QString( path ).append( *eIt ).append( '/' );

    if ( newPath == KGlobalSettings::autostartPath() )
      continue;

    scanDir2( parent, newPath );
  }
}

void KonqDirTree::scanDir2( KonqDirTreeItem *parent, const QString &path )
{
  QDir dir( path );
  QString name = dir.dirName();
  QString icon = "folder";
  bool    open = false;

  kdDebug(1202) << "Scanning " << path << endl;

  QString dotDirectoryFile = QString( path ).append( "/.directory" );

  if ( QFile::exists( dotDirectoryFile ) )
  {
    kdDebug(1202) << "Reading the .directory" << endl;
    KSimpleConfig cfg( dotDirectoryFile, true );
    cfg.setDesktopGroup();
    name = cfg.readEntry( "Name", name );
    icon = cfg.readEntry( "Icon", icon );
    stripIcon( icon );
    open = cfg.readBoolEntry( "Open", open );
  }

  KURL url;
  url.setPath( path );

  KonqFileItem *fileItem = new KonqFileItem( -1, -1, url );
  KonqDirTreeItem *item;
  if ( parent )
    item = new KonqDirTreeItem( this, parent, 0, fileItem );
  else
    item = new KonqDirTreeItem( this, 0, fileItem );
  item->setText( 0, name );
  item->setPixmap( 0, SmallIcon( icon ) );
  item->setListable( false );
  item->setClickable( false );

  kdDebug(1202) << "Inserting group " << name << "   " << path << endl;;
  m_groupItems.insert( item, path );

  item->setOpen( open );

  scanDir( item, path );

  if ( item->childCount() == 0 )
    item->setExpandable( false );
}

void KonqDirTree::loadTopLevelItem( KonqDirTreeItem *parent,  const QString &filename )
{
  KDesktopFile cfg( filename, true );
  cfg.setDollarExpansion(true);

  QFileInfo inf( filename );

  QString path = filename;
  QString icon;
  QString name = KIO::decodeFileName( inf.fileName() );
  if ( name.length() > 8 && name.right( 8 ) == ".desktop" )
    name.truncate( name.length() - 8 );
  if ( name.length() > 7 && name.right( 7 ) == ".kdelnk" )
    name.truncate( name.length() - 7 );

  KURL targetURL;
  targetURL.setPath(path);

  if ( cfg.hasLinkType() )
  {
    targetURL = cfg.readURL();
    icon = cfg.readIcon();
    stripIcon( icon );

    name = cfg.readEntry( "Name", name );
  }
  else if ( cfg.hasDeviceType() )
  {
    // Determine the mountpoint


    // wrong - this needs the device to be already mounted
    //mp = KIO::findDeviceMountPoint( cfg.readEntry( "Dev" ) );
    QString mp = cfg.readEntry("MountPoint");

    if ( mp.isEmpty() )
      return;

    targetURL.setPath(mp);
    icon = cfg.readIcon();
    name = cfg.readEntry( "Name", name );
  }
  else
    return;

  KURL kurl;
  kurl.setPath( path );

  KonqFileItem *fileItem = new KonqFileItem( -1, -1, kurl );
  KonqDirTreeItem *item;
  if ( parent )
      item = new KonqDirTreeItem( this, parent, 0, fileItem );
  else
      item = new KonqDirTreeItem( this, 0, fileItem );

  item->setPixmap( 0, SmallIcon( icon ) );
  item->setText( 0, name );
  item->setLink( true );

  KonqDirLister *dirLister = 0;

  bool bListable = KProtocolInfo::supportsListing( targetURL.protocol() );
  kdDebug() << targetURL.prettyURL() << " listable : " << bListable << endl;

  if ( bListable )
  {
    dirLister = new KonqDirLister( true );
    dirLister->setDirOnlyMode( true );

    connect( dirLister, SIGNAL( newItems( const KFileItemList & ) ),
             this, SLOT( slotNewItems( const KFileItemList & ) ) );
    connect( dirLister, SIGNAL( deleteItem( KFileItem * ) ),
             this, SLOT( slotDeleteItem( KFileItem * ) ) );
    connect( dirLister, SIGNAL( completed() ),
             this, SLOT( slotListingStopped() ) );
    connect( dirLister, SIGNAL( canceled() ),
             this, SLOT( slotListingStopped() ) );
    connect( dirLister, SIGNAL( redirection( const KURL &) ),
             this, SLOT( slotRedirection( const KURL &) ) );
  }
  else
    item->setExpandable( false );

  m_topLevelItems.append( TopLevelItem( item, dirLister, new QMap<KURL, KonqDirTreeItem *>, new KURL::List ) );

  addSubDir( item, item, targetURL );

  bool open = cfg.readBoolEntry( "Open", false /* targetURL.isLocalFile() sucks a bit IMHO - DF */ );

  if ( !bListable )
    item->setListable( false );

  if ( open && item->isExpandable() )
    item->setOpen( true );

}

void KonqDirTree::stripIcon( QString &icon )
{
  QFileInfo info( icon );
  icon = info.baseName();
}

KonqDirTree::TopLevelItem & KonqDirTree::findTopLevelByItem( KonqDirTreeItem *item )
{
  QValueList<TopLevelItem>::Iterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::Iterator end = m_topLevelItems.end();
  for (; it != end; ++it )
   if ( (*it).m_item == item )
     return *it;

  return nullTopLevelItem;
}

KonqDirTree::TopLevelItem & KonqDirTree::findTopLevelByDirLister( const KonqDirLister *lister )
{
  QValueList<TopLevelItem>::Iterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::Iterator end = m_topLevelItems.end();
  for (; it != end; ++it )
   if ( (*it).m_dirLister == lister )
     return *it;

  return nullTopLevelItem;
}

#include "konq_dirtree.moc"
