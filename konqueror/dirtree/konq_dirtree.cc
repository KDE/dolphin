
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
#include <kpopmenu.h>
#include <kio_job.h>
#include <klineeditdlg.h>
#include <kpropsdlg.h>
#include <kuserpaths.h>
#include <kdebug.h>
#include <kio_paste.h>
#include <kglobal.h>

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

  virtual QObject* create( QObject*, const char*, const char*, const QStringList & )
  {
    QObject *obj = new KonqDirTreeBrowserView;
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

KonqDirTreeEditExtension::KonqDirTreeEditExtension( QObject *parent, KonqDirTree *dirTree )
 : EditExtension( parent, "KonqDirTreeEditExtension" )
{
  m_tree = dirTree;
}

void KonqDirTreeEditExtension::can( bool &cut, bool &copy, bool &paste, bool &move )
{
  bool bInTrash = false;

  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  if ( selection && selection->fileItem()->url().directory(false) == KUserPaths::trashPath() )
    bInTrash = true;

  cut = move = copy = selection;
  move = move && !bInTrash;

  bool bKIOClipboard = !isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
	  ( selection );
}

void KonqDirTreeEditExtension::cutSelection()
{
  //TODO: grey out item
  copySelection();
}

void KonqDirTreeEditExtension::copySelection()
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

void KonqDirTreeEditExtension::pasteSelection( bool move )
{
  KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();

  assert( selection );

  pasteClipboard( selection->fileItem()->url().url(), move );
}

void KonqDirTreeEditExtension::moveSelection( const QString &destinationURL )
{
  QStringList lst = selectedUrls();

  if ( lst.count() == 0 )
    return;

  KIOJob *job = new KIOJob;

  if ( !destinationURL.isEmpty() )
    job->move( lst.first(), destinationURL );
  else
    job->del( lst );
}

QStringList KonqDirTreeEditExtension::selectedUrls()
{
   QStringList lst;
   KonqDirTreeItem *selection = (KonqDirTreeItem *)m_tree->selectedItem();
   if (selection)
    // only one item selected
    lst.append( selection->fileItem()->url().url() );

   return lst;
}

KonqDirTreeBrowserView::KonqDirTreeBrowserView( QWidget *parent, const char *name )
  : BrowserView( parent, name )
{
  m_pTree = new KonqDirTree( this );
  setFocusProxy( m_pTree );
  setFocusPolicy( m_pTree->focusPolicy() );

  (void)new KonqDirTreeEditExtension( this, m_pTree );
}

KonqDirTreeBrowserView::~KonqDirTreeBrowserView()
{
}

void KonqDirTreeBrowserView::openURL( const QString &, bool, int, int )
{
  emit started();
  emit completed();
}

QString KonqDirTreeBrowserView::url()
{
  return QDir::homeDirPath().prepend( "file:" );
}

int KonqDirTreeBrowserView::xOffset()
{
  return 0;
}

int KonqDirTreeBrowserView::yOffset()
{
  return 0;
}

void KonqDirTreeBrowserView::stop()
{
}

void KonqDirTreeBrowserView::resizeEvent( QResizeEvent * )
{
  m_pTree->setGeometry( rect() );
}

KonqDirTreeItem::KonqDirTreeItem( KonqDirTree *parent, QListViewItem *parentItem, KonqDirTreeItem *topLevelItem, KFileItem *item )
  : QListViewItem( parentItem )
{
  m_item = item;
  m_topLevelItem = topLevelItem;
  m_tree = parent;

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

  if ( open && !childCount() )
    m_tree->openSubFolder( this, m_topLevelItem );

  QListViewItem::setOpen( open );
}

//TODO: make it configurable via viewprops
static const int autoOpenTimeout = 750;

KonqDirTree::KonqDirTree( KonqDirTreeBrowserView *parent )
  : QListView( parent )
{

  m_folderPixmap = KonqFactory::instance()->iconLoader()->loadApplicationIcon( "folder", KIconLoader::Small );

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

  m_root = new QListViewItem( this, i18n( "Computer" ) );
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

  if ( topLevelItem.m_dirLister->jobId() == 0 )
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
  if ( QUrlDrag::decodeToUnicodeUris( ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      kdebug(KDEBUG_WARN,1202,"Oooops, no data ....");
      return;
    }
    KIOJob* job = new KIOJob;

    KURL dest( selection->fileItem()->url() );

    switch ( ev->action() ) {
      case QDropEvent::Move : job->move( lst, dest.url( 1 ) ); break;
      case QDropEvent::Copy : job->copy( lst, dest.url( 1 ) ); break;
      //      case QDropEvent::Link : {
      //        link( lst, dest );
      //        break;
      //      }
      default : kdebug( KDEBUG_ERROR, 1202, "Unknown action %d", ev->action() ); return;
    }
  }
  else if ( formats.count() >= 1 )
  {
    kdebug(0,1202,"Pasting to %s", selection->fileItem()->url().url().ascii() /* item's url */);
    pasteData( selection->fileItem()->url().url()/* item's url */, ev->data( formats.first() ) );
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

void KonqDirTree::slotNewItem( KFileItem *item )
{
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
  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  if ( m_groupItems.find( item ) != m_groupItems.end() )
    return;

  emit m_view->openURLRequest( ((KonqDirTreeItem *)item)->fileItem()->url().url(), false, 0, 0 );
}

void KonqDirTree::slotRightButtonPressed( QListViewItem *item )
{
  if ( !item )
    return;

  QMap<QListViewItem *,QString>::ConstIterator groupItem = m_groupItems.find( item );
  if ( groupItem != m_groupItems.end() )
  {
    groupPopupMenu( *groupItem, item );
    return;
  }

  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  item->setSelected( true );

  KFileItemList lstItems;

  lstItems.append( ((KonqDirTreeItem *)item)->fileItem() );

  emit m_view->popupMenu( QCursor::pos(), lstItems );
}

void KonqDirTree::slotClicked( QListViewItem *item )
{
  if ( !item )
    return;

  if ( m_unselectableItems.findRef( item ) != -1 )
    return;

  emit m_view->openURLRequest( ((KonqDirTreeItem *)item)->fileItem()->url().url(), false, 0, 0 );
}

void KonqDirTree::slotListingStopped()
{
  KDirLister *lister = (KDirLister *)sender();

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  KURL url = lister->kurl();

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

  QStringList dirs = KonqFactory::instance()->dirs()->findDirs( "data", "konqueror/dirtree/" );
  QStringList::ConstIterator dIt = dirs.begin();
  QStringList::ConstIterator dEnd = dirs.end();

  for (; dIt != dEnd; ++dIt )
    scanDir( m_root, *dIt );
}

void KonqDirTree::scanDir( QListViewItem *parent, const QString &path )
{
  QDir dir( path );

  if ( !dir.isReadable() )
    return;

  QStringList entries = dir.entryList( QDir::Files );
  QStringList::ConstIterator eIt = entries.begin();
  QStringList::ConstIterator eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
    loadTopLevelItem( parent, QString( *eIt ).prepend( path ) );

  entries = dir.entryList( QDir::Dirs );
  eIt = entries.begin();
  eEnd = entries.end();

  for (; eIt != eEnd; eIt++ )
  {
    if ( *eIt == "." || *eIt == ".." )
      continue;

    scanDir2( parent, QString( path ).append( *eIt ) );
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
    name = cfg.readEntry( "Name" );
    icon = cfg.readEntry( "Icon" );
  }

  QListViewItem *item = new QListViewItem( parent );
  item->setText( 0, name );
  item->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadApplicationIcon( icon, KIconLoader::Small ) );
  item->setSelectable( false );

  m_unselectableItems.append( item );

  QString groupPath = QString( path ).append( "/" );

  m_groupItems.insert( item, groupPath );

  scanDir( item, groupPath );
}

void KonqDirTree::loadTopLevelItem( QListViewItem *parent,  const QString &filename )
{
  KSimpleConfig cfg( filename, true );

  cfg.setDesktopGroup();

  QString url = cfg.readEntry( "URL" );
  QString icon = cfg.readEntry( "Icon" );
  QString name = cfg.readEntry( "Name" );

  if ( url == "file:$HOME" ) //HACK
    url = QDir::homeDirPath().prepend( "file:" );

  KFileItem *fileItem = new KFileItem( 0, KURL( url ) );
  KonqDirTreeItem *item = new KonqDirTreeItem( this, parent, 0, fileItem );

  //  m_unselectableItems.append( item );

  //  item->setSelectable( false );

  item->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadApplicationIcon( icon, KIconLoader::Small ) );
  item->setText( 0, name );

  KDirLister *dirLister = new KDirLister;
  dirLister->setDirOnlyMode( true );

  connect( dirLister, SIGNAL( newItem( KFileItem * ) ),
	   this, SLOT( slotNewItem( KFileItem * ) ) );
  connect( dirLister, SIGNAL( deleteItem( KFileItem * ) ),
	   this, SLOT( slotDeleteItem( KFileItem * ) ) );
  connect( dirLister, SIGNAL( completed() ),
	   this, SLOT( slotListingStopped() ) );
  connect( dirLister, SIGNAL( canceled() ),
	   this, SLOT( slotListingStopped() ) );

  m_topLevelItems.append( TopLevelItem( item, dirLister, new QMap<KURL, KonqDirTreeItem *>, new KURL::List ) );
  addSubDir( item, item, fileItem->url().url( 0 ) );

  if ( cfg.readBoolEntry( "Open", true ) )
    item->setOpen( true );

}

void KonqDirTree::groupPopupMenu( const QString &path, QListViewItem *item )
{
  return; //disabled for now (because it's not really that functional ;(

  KPopupMenu *popup = new KPopupMenu;

  popup->insertTitle( i18n( "Tree Options for \"%1\"" ).arg( item->text( 0 ) ) );

  popup->insertSeparator();

  popup->insertItem( i18n( "Add Group" ), 1);
  popup->insertItem( i18n( "Add Link" ), 2 );
  popup->insertSeparator();
  popup->insertItem( i18n( "Remove Group" ), 3 );

  int result = popup->exec( QCursor::pos() );

  switch ( result ) {
  case 1: addGroup( path, item ); break;
  case 2: addLink( path, item ); break;
  case 3: removeGroup( path, item ); break;
  default: return;
  }
}

void KonqDirTree::addGroup( const QString &path, QListViewItem *item )
{
  KLineEditDlg l( i18n( "New Group" ), QString::null, this );

  if ( !l.exec() || l.text().isEmpty() )
    return;

  QString newGroupPath = path;
  newGroupPath.append( '/' );
  newGroupPath.append( l.text() );

  QDir dir;
  dir.mkdir( newGroupPath );

  scanDir2( item, newGroupPath );
}

void KonqDirTree::addLink( const QString &path, QListViewItem *item )
{
  //TODO
}

void KonqDirTree::removeGroup( const QString &path, QListViewItem *item )
{
  delete item;
  // we're lazy ;-) let kio_file do the work of removing all the recursive stuff ;-)
  KIOJob *job = new KIOJob;
  job->del( QString( path ).prepend( "file:" ) );
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

