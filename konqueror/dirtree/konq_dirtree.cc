
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
#include <konqdirlister.h>
#include <kdirwatch.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <klibloader.h>
#include <klocale.h>
#include <konqfileitem.h>
#include <konqoperations.h>
#include <konqsettings.h>
#include <kparts/factory.h>
#include <kprotocolmanager.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

#include <assert.h>

/*
  TODO: - merge with KonqTreeView?
 */

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
      delete s_instance;
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

  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  if ( selection && selection->fileItem()->url().directory(false) == KGlobalSettings::trashPath() )
    bInTrash = true;

  cutcopy = del = selection;

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "trash", del && !bInTrash );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
	       ( selection );

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
  KIO::pasteClipboard( selection->fileItem()->url(), move );
}

KURL::List KonqDirTreeBrowserExtension::selectedUrls()
{
  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();
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
  m_url = KURL( QDir::homeDirPath().prepend( "file:" ) );
}

KonqDirTreePart::~KonqDirTreePart()
{
}

bool KonqDirTreePart::openURL( const KURL & url )
{
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

KonqDirTreeItem::KonqDirTreeItem( KonqDirTree *parent, QListViewItem *parentItem, KonqDirTreeItem *topLevelItem, KonqFileItem *item )
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

  m_folderPixmap = SmallIcon("folder");

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
  m_root->setPixmap( 0, SmallIcon("desktop") );
  m_root->setSelectable( false );

  connect( this, SIGNAL( doubleClicked( QListViewItem * ) ),
	   this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
	   this, SLOT( slotRightButtonPressed( QListViewItem * ) ) );
  connect( this, SIGNAL( clicked( QListViewItem * ) ),
	   this, SLOT( slotClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( returnPressed( QListViewItem * ) ),
	   this, SLOT( slotDoubleClicked( QListViewItem * ) ) );

  m_lastItem = 0L;

  QString dirtreeDir = KonqDirTreeFactory::instance()->dirs()->saveLocation( "data", "konqueror/dirtree/" );
  m_pDirWatch = new KDirWatch();
  m_pDirWatch->addDir( dirtreeDir );
  slotScanDir( dirtreeDir );

  connect( m_pDirWatch, SIGNAL( dirty( const QString & ) ),
           this, SLOT( slotScanDir( const QString & ) ) );
}

KonqDirTree::~KonqDirTree()
{
  clear();
  delete m_pDirWatch;
}

void KonqDirTree::clear()
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
            m_selectAfterOpening = _url;
	    //kdDebug(1202) << "KonqDirTree::followURL: m_selectAfterOpening=" << m_selectAfterOpening.url() << endl;
            dirIt.data()->setOpen( true );
            return; // We know we won't find it
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

  KonqOperations::doDrop( selection->fileItem(), ev, this );
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

  QStringList lst;
  lst << ((KonqDirTreeItem *)item)->fileItem()->url().url();

  QUriDrag *drag = new QUriDrag( viewport() );
  drag->setUnicodeUris( lst );
  drag->drag();
}

void KonqDirTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
  KListView::contentsMouseReleaseEvent( e );
  m_bDrag = false;
}

void KonqDirTree::slotNewItems( const KFileItemList& entries )
{
  kdDebug() << "slotNewItems " << entries.count() << endl;

  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  QListIterator<KFileItem> kit ( entries );
  for( ; kit.current(); ++kit )
  {
    KonqFileItem * item = static_cast<KonqFileItem *>(*kit);

    assert( S_ISDIR( item->mode() ) );

    KURL dir( item->url() );
    dir.setFileName( "" );

    //  KonqDirTreeItem *parentDir = findDir( *topLevelItem.m_mapSubDirs, dir.url( 0 ) );
    //  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->find( dir );
    // *mumble* can't use QMap::find() because the cmp doesn't ignore the trailing slash :-(
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
    dirTreeItem->setText( 0, KIO::decodeFileName( item->url().filename() ) );
  }
}

void KonqDirTree::slotDeleteItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  //  qDebug( "slotDeleteItem( %s )", item->url().url().ascii() );

  KonqDirLister *lister = (KonqDirLister *)sender();

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

  KonqDirTreeItem *dItem = static_cast<KonqDirTreeItem *>( item );

  KParts::URLArgs args;

  if ( dItem->isListable() )
    args.serviceType = QString::fromLatin1( "inode/directory" );

  emit m_view->extension()->openURLRequest( dItem->fileItem()->url(), args );

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

  lstItems.append( (static_cast<KonqDirTreeItem *>(item))->fileItem() );

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

  KonqDirTreeItem *dItem = static_cast<KonqDirTreeItem *>( item );

  KParts::URLArgs args;

  if ( dItem->isListable() )
    args.serviceType = QString::fromLatin1( "inode/directory" );

  emit m_view->extension()->openURLRequest( dItem->fileItem()->url(), args );

  m_lastItem = item;
}

void KonqDirTree::slotListingStopped()
{
  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  KURL url = lister->url();

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
    topLevelItem.m_dirLister->openURL( topLevelItem.m_lstPendingURLs->first(), false, true );

  QMap<KURL, QListViewItem *>::Iterator oIt = m_mapCurrentOpeningFolders.find( url );
  if ( oIt != m_mapCurrentOpeningFolders.end() )
  {
    oIt.data()->setPixmap( 0, m_folderPixmap );
    if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL() == url )
    {
      followURL( m_selectAfterOpening );
      m_selectAfterOpening = KURL();
    }

    m_mapCurrentOpeningFolders.remove( oIt );

    if ( m_mapCurrentOpeningFolders.count() == 0 )
      m_animationTimer->stop();
  }
}

void KonqDirTree::slotAnimation()
{
  QPixmap gearPixmap = BarIcon( QString::fromLatin1( "kde" ).append( QString::number( m_animationCounter ) ), KonqDirTreeFactory::instance() );

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

void KonqDirTree::slotScanDir( const QString & dir )
{
  kdDebug(1202) << "KonqDirTree::slotScanDir " << dir << endl;
  clear();
  m_autoOpenTimer->stop();
  m_topLevelItems.clear();
  m_groupItems.clear();
  m_mapCurrentOpeningFolders.clear();
  m_unselectableItems.clear();
  m_unselectableItems.append( m_root );
  QString path( dir );
  if ( path.right(1) != "/" ) // first call has the /, but KDirWatch doesn't emit it
      path += "/";
  scanDir( m_root, path, true);
  m_root->setOpen( true );
}

void KonqDirTree::scanDir( QListViewItem *parent, const QString &path, bool isRoot )
{
  QDir dir( path );

  if ( !dir.isReadable() )
    return;

  QStringList entries = dir.entryList( QDir::Files );

  if ( isRoot && entries.count() == 0 )
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

    dir.setPath( path ); //hack to make QDir to consider the dir to be dirty and re-read it
    entries = dir.entryList( QDir::Files );
  }

  QStringList::ConstIterator eIt = entries.begin();
  QStringList::ConstIterator eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
  {
    QString filePath = QString( *eIt ).prepend( path );
    if ( KMimeType::findByURL( filePath, 0, true )->name() == "application/x-desktop" )
      loadTopLevelItem( parent, filePath );
  }

  entries = dir.entryList( QDir::Dirs );
  eIt = entries.begin();
  eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
  {
    QString newPath = QString( path ).append( *eIt ).append( '/' );

    if ( *eIt == "." || *eIt == ".." || newPath == KGlobalSettings::autostartPath() )
      continue;

    scanDir2( parent, newPath );
  }
}

void KonqDirTree::scanDir2( QListViewItem *parent, const QString &path )
{
  QDir dir( path );
  QString name = dir.dirName();
  QString icon = "folder";
  bool    open = false;

  qDebug( "Scanning %s",  path.ascii() );

  QString dotDirectoryFile = QString(  path ).append( "/.directory" );

  if ( QFile::exists( dotDirectoryFile ) )
  {
    KSimpleConfig cfg( dotDirectoryFile, true );
    cfg.setDesktopGroup();
    name = cfg.readEntry( "Name", name );
    icon = cfg.readEntry( "Icon", icon );
    stripIcon( icon );
    open = cfg.readBoolEntry( "Open", open );
  }

  QString url = QString( path ).prepend( "file:" );

  KonqFileItem *fileItem = new KonqFileItem( -1, -1, KURL( url ) );
  KonqDirTreeItem *item = new KonqDirTreeItem( this, parent, 0, fileItem );
  //  QListViewItem *item = new QListViewItem( parent );
  item->setText( 0, name );
  item->setPixmap( 0, SmallIcon( icon ) );
  item->setListable( false );
  //  item->setSelectable( false );

  //  m_unselectableItems.append( item );

  QString groupPath = QString( path ).append( "/" );

  m_groupItems.insert( item, groupPath );

  item->setOpen( open );

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

  KURL kurl( url );
  if ( kurl.path().isEmpty() )
    kurl.setPath( "/" );

  KonqFileItem *fileItem = new KonqFileItem( -1, -1, kurl );
  KonqDirTreeItem *item = new KonqDirTreeItem( this, parent, 0, fileItem );

  //  m_unselectableItems.append( item );

  //  item->setSelectable( false );

  item->setPixmap( 0, SmallIcon( icon ) );
  item->setText( 0, name );

  KonqDirLister *dirLister = 0;

  bool bListable = KProtocolManager::self().supportsListing( kurl.protocol() );

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
  }
  else
    item->setExpandable( false );

  m_topLevelItems.append( TopLevelItem( item, dirLister, new QMap<KURL, KonqDirTreeItem *>, new KURL::List ) );

  addSubDir( item, item, fileItem->url() );

  bool hasOpenKey = cfg.hasKey( "Open" );
  bool open = cfg.readBoolEntry( "Open", true );

  if ( !bListable )
    item->setListable( false );

  if ( ( ( !hasOpenKey && fileItem->url().isLocalFile() ) || ( hasOpenKey && open ) ) && item->isExpandable() )
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

KonqDirTree::TopLevelItem KonqDirTree::findTopLevelByDirLister( const KonqDirLister *lister )
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

