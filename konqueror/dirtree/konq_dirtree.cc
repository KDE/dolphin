
#include "konq_dirtree.h"
#include "konq_factory.h"

#include <qdir.h>
#include <qheader.h>
#include <qcursor.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qdragobject.h>
#include <qapplication.h>
#include <qclipboard.h>

#include <ksimpleconfig.h>
#include <kstddirs.h>
#include <kiconloader.h>
#include <klibloader.h>
#include <kinstance.h>
#include <kfileitem.h>
#include <kdirlister.h>
#include <klocale.h>
#include <kio/job.h>
#include <kuserpaths.h>
#include <kdebug.h>
#include <kio/paste.h>
#include <kglobal.h>
#include <kdesktopfile.h>
#include <konqsettings.h>

#include <assert.h>

/*
  TODO: - install KDirWatch'es on the treeview configuration dirs
        - merge with KonqTreeView?
 */

inline bool operator<( const KURL &u1, const KURL &u2 )
{
  return u1.url( 0 ) < u2.url( 0 );
}

class KonqDirTreeFactory : public KLibFactory
{
public:
  KonqDirTreeFactory()
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqDirTreeFactory()
  {
    KonqFactory::instanceUnref();
  }

  virtual QObject* create( QObject *parent, const char *name, const char*, const QStringList & )
  {
    QObject *obj = new KonqDirTreePart( (QWidget *)parent, parent, name );
    emit objectCreated( obj );
    return obj;
  }

};

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
  bool bInTrash = false;

  bool cutcopy, move;

  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  if ( selection && selection->fileItem()->url().directory(false) == KUserPaths::trashPath() )
    bInTrash = true;

  cutcopy = move = selection;
  move = move && !bInTrash;

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
	       ( selection );

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "del", move );
  emit enableAction( "trash", move );
  emit enableAction( "pastecut", paste );
  emit enableAction( "pastecopy", paste );
}

void KonqDirTreeBrowserExtension::cut()
{
  //TODO: grey out item
  copy();
}

void KonqDirTreeBrowserExtension::copy()
{
  QStringList lst;

  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  if ( !selection )
    return;

  lst.append( selection->fileItem()->url().url() );

  QUriDrag *drag = new QUriDrag( m_tree->viewport() );
  drag->setUnicodeUris( lst );
  QApplication::clipboard()->setData( drag );
}

void KonqDirTreeBrowserExtension::pasteSelection( bool move )
{
  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  assert( selection );

  KIO::pasteClipboard( selection->fileItem()->url().url(), move );
}

void KonqDirTreeBrowserExtension::moveSelection( const QString &destinationURL )
{
  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  if ( !selection )
    return;

  KIO::Job * job = 0L;

  if ( !destinationURL.isEmpty() )
    job = KIO::move( selection->fileItem()->url(), destinationURL );
  else
    job = KIO::del( selection->fileItem()->url() );

  connect( job, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotResult( KIO::Job * ) ) );
}

void KonqDirTreeBrowserExtension::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
}

KonqDirTreePart::KonqDirTreePart( QWidget *parentWidget, QObject *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
  m_pTree = new KonqDirTree( this, parentWidget );

  m_extension = new KonqDirTreeBrowserExtension( this, m_pTree );

  setWidget( m_pTree );
  setInstance( KonqFactory::instance(), false );
  m_url = KURL( QDir::homeDirPath().prepend( "file:" ) );
}

KonqDirTreePart::~KonqDirTreePart()
{
}

bool KonqDirTreePart::openURL( const KURL & )
{
  emit started( 0 );
  emit completed();
  return true;
}

bool KonqDirTreePart::closeURL()
{
  // (David) shouldn't we stop the dirLister here?

  return true;
}

bool KonqDirTreePart::event( QEvent *e )
{
 if ( KParts::ReadOnlyPart::event( e ) )
   return true;

 if ( KParts::OpenURLEvent::test( e ) && ((KParts::OpenURLEvent *)e)->part() != this && KonqFMSettings::defaultIconSettings()->treeFollow() )
 {
   m_pTree->followURL( ((KParts::OpenURLEvent *)e)->url() );
   return true;
 }

 return false;
}

KonqDirTreeItem::KonqDirTreeItem( KonqDirTree *parent, QListViewItem *parentItem, KonqDirTreeItem *topLevelItem, KFileItem *item )
  : QListViewItem( parentItem )
{
  m_item = item;
  m_topLevelItem = topLevelItem;
  m_tree = parent;
  m_bListable = true;

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

  if ( open && !childCount() && m_bListable )
    m_tree->openSubFolder( this, m_topLevelItem );

  QListViewItem::setOpen( open );
}

void KonqDirTreeItem::setListable( bool b )
{
  m_bListable = b;
}

//TODO: make it configurable via viewprops
static const int autoOpenTimeout = 750;

KonqDirTree::KonqDirTree( KonqDirTreePart *parent, QWidget *parentWidget )
  : KListView( parentWidget )
{

  m_folderPixmap = KonqFactory::instance()->iconLoader()->loadIcon( "folder", KIconLoader::Small );

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

  m_root = new QListViewItem( this, i18n( "Desktop" ) );
  m_root->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadIcon( "desktop", KIconLoader::Small ) );
  m_root->setSelectable( false );

  connect( this, SIGNAL( doubleClicked( QListViewItem * ) ),
	   this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
	   this, SLOT( slotRightButtonPressed( QListViewItem * ) ) );
  connect( this, SIGNAL( clicked( QListViewItem * ) ),
	   this, SLOT( slotClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( returnPressed( QListViewItem * ) ),
	   this, SLOT( slotDoubleClicked( QListViewItem * ) ) );

  m_unselectableItems.append( m_root );

  m_lastItem = 0L;

  init();

  m_root->setOpen( true );
}

KonqDirTree::~KonqDirTree()
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

}

void KonqDirTree::openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel )
{
//  qDebug( "openSubFolder( %s )", item->fileItem()->url().url().ascii() );
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  KURL u = item->fileItem()->url();

  if ( topLevelItem.m_dirLister->job() == 0 )
    topLevelItem.m_dirLister->openURL( u, false, topLevel ? true : false );
  else  if ( !topLevelItem.m_lstPendingURLs->contains( u ) )
    topLevelItem.m_lstPendingURLs->append( u );

  if ( !topLevel )
    return;

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
  KURL u( _url );
  QValueList<TopLevelItem>::ConstIterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::ConstIterator end = m_topLevelItems.end();
  for (; it != end; ++it )
  {
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = (*it).m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = (*it).m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
      if ( u.cmp( dirIt.key(), true ) )
      {
        if ( !dirIt.data()->isOpen() )
	  dirIt.data()->setOpen( true );
	
	ensureItemVisible( dirIt.data() );
	
        return;
      }
  }
}

void KonqDirTree::contentsDragEnterEvent( QDragEnterEvent * )
{
  m_dropItem = 0;
}

void KonqDirTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
  QListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

  if ( !item || !item->isSelectable() )
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

  KonqDirTreeItem *selection = (KonqDirTreeItem *)selectedItem();

  assert( selection );

  QStringList lst;

  QStringList formats;

  for ( int i = 0; ev->format( i ); i++ )
    if ( *( ev->format( i ) ) )
      formats.append( ev->format( i ) );

  // Try to decode to the data you understand...
  if ( QUriDrag::decodeToUnicodeUris( ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      kDebugWarning(1202,"Oooops, no data ....");
      return;
    }
    KIO::Job* job = 0L;

    KURL dest( selection->fileItem()->url() );

    switch ( ev->action() ) {
        case QDropEvent::Move : job = KIO::move( lst, dest.url( 1 ) ); break;
        case QDropEvent::Copy : job = KIO::copy( lst, dest.url( 1 ) ); break;
        case QDropEvent::Link : KIO::link( lst, dest ); break;
        default : kDebugError( 1202, "Unknown action %d", ev->action() ); return;
    }
    connect( job, SIGNAL( result( KIO::Job * ) ),
             SLOT( slotResult( KIO::Job * ) ) );
    ev->acceptAction(TRUE);
    ev->accept();
  }
  else if ( formats.count() >= 1 )
  {
    kDebugInfo(1202,"Pasting to %s", selection->fileItem()->url().url().ascii() /* item's url */);
    KIO::pasteData( selection->fileItem()->url().url()/* item's url */, ev->data( formats.first() ) );
  }

}

void KonqDirTree::contentsMousePressEvent( QMouseEvent *e )
{
  QListView::contentsMousePressEvent( e );
  m_dragPos = e->pos();
  m_bDrag = true;
}

void KonqDirTree::contentsMouseMoveEvent( QMouseEvent *e )
{
  if ( !m_bDrag || ( e->pos() - m_dragPos ).manhattanLength() <= KGlobal::dndEventDelay() )
    return;

  m_bDrag = false;

  QListViewItem *item = itemAt( contentsToViewport( m_dragPos ) );

  if ( !item || !item->isSelectable() )
    return;

  QStringList lst;
  lst << ((KonqDirTreeItem *)item)->fileItem()->url().url();

  QUriDrag *drag = new QUriDrag( viewport() );
  drag->setUnicodeUris( lst );
  drag->drag();
}

void KonqDirTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
  QListView::contentsMouseReleaseEvent( e );
  m_bDrag = false;
}

void KonqDirTree::slotNewItems( const KFileItemList& entries )
{
  QListIterator<KFileItem> kit ( entries );
  for( ; kit.current(); ++kit )
  {
    KFileItem * item = *kit;

    assert( S_ISDIR( item->mode() ) );

    KDirLister *lister = (KDirLister *)sender();

    TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

    assert( topLevelItem.m_item );

    KURL dir( item->url() );
    dir.setFileName( "" );

    //  KonqDirTreeItem *parentDir = findDir( *topLevelItem.m_mapSubDirs, dir.url( 0 ) );
    //  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->find( dir );
    // *mumble* can't use QMap::find() because the cmp doesn't ingore the trailing slash :-(
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = topLevelItem.m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
    {
      //    qDebug( "comparing %s with %s", dirIt.key().url().ascii(), dir.url().ascii() );
      if ( dir.cmp( dirIt.key(), true ) )
        break;
    }

    assert( dirIt != topLevelItem.m_mapSubDirs->end() );

    KonqDirTreeItem *parentDir = dirIt.data();

    assert( parentDir );

    KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( this, parentDir, topLevelItem.m_item, item );
    dirTreeItem->setPixmap( 0, m_folderPixmap );
    dirTreeItem->setText( 0, item->url().filename() );
  }
}

void KonqDirTree::slotDeleteItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  //  qDebug( "slotDeleteItem( %s )", item->url().url().ascii() );

  KDirLister *lister = (KDirLister *)sender();

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  if ( item == topLevelItem.m_item->fileItem() )
  {
    assert( 0 ); //TODO ;)
  }

  QListViewItemIterator it( topLevelItem.m_item );
  for (; it.current(); ++it )
  {
    if ( ((KonqDirTreeItem *)it.current())->fileItem() == item )
    {
    //      qDebug( "removing %s", item->url().url().ascii() );
      delete it.current();
      return;
    }
  }

}

void KonqDirTree::slotDoubleClicked( QListViewItem *item )
{
  if ( !item )
    return;
 
  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  if ( m_groupItems.find( item ) != m_groupItems.end() )
    return;

  if ( item == m_lastItem )
    return;

  emit m_view->extension()->openURLRequest( ((KonqDirTreeItem *)item)->fileItem()->url(), false, 0, 0 );

  m_lastItem = item;
}

void KonqDirTree::slotRightButtonPressed( QListViewItem *item )
{
  if ( !item )
    return;

  QMap<QListViewItem *,QString>::ConstIterator groupItem = m_groupItems.find( item );
  if ( groupItem != m_groupItems.end() )
    return;

  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  item->setSelected( true );

  KFileItemList lstItems;

  lstItems.append( ((KonqDirTreeItem *)item)->fileItem() );

  emit m_view->extension()->popupMenu( QCursor::pos(), lstItems );
}

void KonqDirTree::slotClicked( QListViewItem *item )
{
  if ( !item )
    return;

  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  if ( item == m_lastItem )
    return;

  emit m_view->extension()->openURLRequest( ((KonqDirTreeItem *)item)->fileItem()->url(), false, 0, 0 );

  m_lastItem = item;
}

void KonqDirTree::slotListingStopped()
{
  KDirLister *lister = (KDirLister *)sender();

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  KURL url = lister->url();

  KURL::List::Iterator it = topLevelItem.m_lstPendingURLs->find( url );
  if ( it != topLevelItem.m_lstPendingURLs->end() )
    topLevelItem.m_lstPendingURLs->remove( it );

  if ( topLevelItem.m_lstPendingURLs->count() > 0 )
    topLevelItem.m_dirLister->openURL( topLevelItem.m_lstPendingURLs->first(), false, true );

  QMap<KURL, QListViewItem *>::Iterator oIt = m_mapCurrentOpeningFolders.find( url );
  if ( oIt != m_mapCurrentOpeningFolders.end() )
  {
    oIt.data()->setPixmap( 0, m_folderPixmap );

    m_mapCurrentOpeningFolders.remove( oIt );

    if ( m_mapCurrentOpeningFolders.count() == 0 )
      m_animationTimer->stop();
  }
}

void KonqDirTree::slotAnimation()
{
  QPixmap gearPixmap = BarIcon( QString::fromLatin1( "kde" ).append( QString::number( m_animationCounter ) ), KonqFactory::instance() );

  QMap<KURL, QListViewItem *>::ConstIterator it = m_mapCurrentOpeningFolders.begin();
  QMap<KURL, QListViewItem *>::ConstIterator end = m_mapCurrentOpeningFolders.end();
  for (; it != end; ++it )
    it.data()->setPixmap( 0, gearPixmap );

  m_animationCounter++;
  if ( m_animationCounter == 10 )
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

void KonqDirTree::init()
{
  scanDir( m_root, KonqFactory::instance()->dirs()->saveLocation( "data", "konqueror/dirtree/" ), true );
}

void KonqDirTree::scanDir( QListViewItem *parent, const QString &path, bool isRoot )
{
  QDir dir( path );

  if ( !dir.isReadable() )
    return;

  QStringList entries = dir.entryList( QDir::Files );

  if ( isRoot && entries.count() == 0 )
  {
    QString homeLnk = locate( "data", "konqueror/dirtree/home.desktop", KonqFactory::instance() );

    if ( !homeLnk.isEmpty() )
    {
      QCString cp;
      cp.sprintf( "cp %s %s", homeLnk.local8Bit().data(), path.local8Bit().data() );
      system( cp.data() );
    }

    QString rootLnk = locate( "data", "konqueror/dirtree/root.desktop", KonqFactory::instance() );

    if ( !rootLnk.isEmpty() )
    {
      QCString cp;
      cp.sprintf( "cp %s %s", rootLnk.local8Bit().data(), path.local8Bit().data() );
      system( cp.data() );
    }

    dir.setPath( path ); //hack to make QDir to consider the dir to be dirty and re-read it
    entries = dir.entryList( QDir::Files );
  }

  QStringList::ConstIterator eIt = entries.begin();
  QStringList::ConstIterator eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
    if ( KDesktopFile::isDesktopFile( *eIt ) )
      loadTopLevelItem( parent, QString( *eIt ).prepend( path ) );

  entries = dir.entryList( QDir::Dirs );
  eIt = entries.begin();
  eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
  {
    QString newPath = QString( path ).append( *eIt ).append( '/' );

    if ( *eIt == "." || *eIt == ".." || newPath == KUserPaths::templatesPath() || newPath == KUserPaths::autostartPath() )
      continue;

    scanDir2( parent, newPath );
  }
}

void KonqDirTree::scanDir2( QListViewItem *parent, const QString &path )
{
  QDir dir( path );
  QString name = dir.dirName();
  QString icon = "folder";

  qDebug( "Scanning %s",  path.ascii() );

  QString dotDirectoryFile = QString(  path ).append( "/.directory" );

  if ( QFile::exists( dotDirectoryFile ) )
  {
    KSimpleConfig cfg( dotDirectoryFile, true );
    cfg.setDesktopGroup();
    name = cfg.readEntry( "Name", name );
    icon = cfg.readEntry( "Icon" );
    stripIcon( icon );
  }

  QString url = QString( path ).prepend( "file:" );

  KFileItem *fileItem = new KFileItem( -1, -1, KURL( url ) );
  KonqDirTreeItem *item = new KonqDirTreeItem( this, parent, 0, fileItem );
  //  QListViewItem *item = new QListViewItem( parent );
  item->setText( 0, name );
  item->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadIcon( icon, KIconLoader::Small ) );
  item->setListable( false );
  //  item->setSelectable( false );

  //  m_unselectableItems.append( item );

  QString groupPath = QString( path ).append( "/" );

  m_groupItems.insert( item, groupPath );

  item->setOpen( true );

  scanDir( item, groupPath );
}

void KonqDirTree::loadTopLevelItem( QListViewItem *parent,  const QString &filename )
{
  KDesktopFile cfg( filename, true );

  QFileInfo inf( filename );

  QString url, icon;
  QString name = inf.baseName();

  if ( cfg.hasLinkType() )
  {
    url = cfg.readURL();
    icon = cfg.readIcon();

    stripIcon( icon );

    name = cfg.readEntry( "Name", name );

    if ( url == "file:$HOME" ) //HACK
      url = QDir::homeDirPath().prepend( "file:" );
  }
  else if ( cfg.hasDeviceType() )
  {
    QString mountPoint = KIO::findDeviceMountPoint( cfg.readEntry( "Dev" ) );

    if ( mountPoint.isNull() )
      return;

    url = mountPoint.prepend( "file:" );
    icon = cfg.readIcon();
    name = cfg.readEntry( "Name", name );
  }
  else
    return;

  KFileItem *fileItem = new KFileItem( -1, -1, KURL( url ) );
  KonqDirTreeItem *item = new KonqDirTreeItem( this, parent, 0, fileItem );

  //  m_unselectableItems.append( item );

  //  item->setSelectable( false );

  item->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadIcon( icon, KIconLoader::Small ) );
  item->setText( 0, name );

  KDirLister *dirLister = new KDirLister( true );
  dirLister->setDirOnlyMode( true );

  connect( dirLister, SIGNAL( newItems( const KFileItemList & ) ),
	   this, SLOT( slotNewItems( const KFileItemList & ) ) );
  connect( dirLister, SIGNAL( deleteItem( KFileItem * ) ),
	   this, SLOT( slotDeleteItem( KFileItem * ) ) );
  connect( dirLister, SIGNAL( completed() ),
	   this, SLOT( slotListingStopped() ) );
  connect( dirLister, SIGNAL( canceled() ),
	   this, SLOT( slotListingStopped() ) );

  m_topLevelItems.append( TopLevelItem( item, dirLister, new QMap<KURL, KonqDirTreeItem *>, new KURL::List ) );

  addSubDir( item, item, fileItem->url() );

  if ( fileItem->url().isLocalFile() )
    item->setOpen( true );

}

void KonqDirTree::stripIcon( QString &icon )
{
  QFileInfo info( icon );
  icon = info.baseName();
}

KonqDirTree::TopLevelItem KonqDirTree::findTopLevelByItem( KonqDirTreeItem *item )
{
  QValueList<TopLevelItem>::ConstIterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::ConstIterator end = m_topLevelItems.end();
  for (; it != end; ++it )
   if ( (*it).m_item == item )
     return *it;

  return TopLevelItem();
}

KonqDirTree::TopLevelItem KonqDirTree::findTopLevelByDirLister( KDirLister *lister )
{
  QValueList<TopLevelItem>::ConstIterator it = m_topLevelItems.begin();
  QValueList<TopLevelItem>::ConstIterator end = m_topLevelItems.end();
  for (; it != end; ++it )
   if ( (*it).m_dirLister == lister )
     return *it;

  return TopLevelItem();
}

// DO NOT REMOVE THIS INCLUDE!!!
#include "konq_dirtree.moc"

