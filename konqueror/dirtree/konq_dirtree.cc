
#include "konq_dirtree.h"
#include "konq_factory.h"

#include <qdir.h>
#include <qheader.h>
#include <qcursor.h>
#include <qfileinfo.h>

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

#include <assert.h>

/*
  TODO: - install KDirWatch'es on the treeview configuration dirs
        - merge with KonqTreeView?
 */

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


KonqDirTreeBrowserView::KonqDirTreeBrowserView( QWidget *parent, const char *name )
  : BrowserView( parent, name )
{
  m_pTree = new KonqDirTree( this );
}

KonqDirTreeBrowserView::~KonqDirTreeBrowserView()
{
}

void KonqDirTreeBrowserView::openURL( const QString &url, bool reload, int xOffset, int yOffset )
{
}

QString KonqDirTreeBrowserView::url()
{
  return QString::null;
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
    m_tree->addSubDir( this, m_topLevelItem, m_item->url().url( 0 ) );

  setExpandable( true );
}

KonqDirTreeItem::~KonqDirTreeItem()
{
  if ( m_topLevelItem )
    m_tree->removeSubDir( this, m_topLevelItem, m_item->url().url( 0 ) );
}

void KonqDirTreeItem::setOpen( bool open )
{

  if ( open && !childCount() )
    m_tree->openSubFolder( this, m_topLevelItem );

  QListViewItem::setOpen( open );
}

KonqDirTree::KonqDirTree( KonqDirTreeBrowserView *parent )
  : QListView( parent )
{

  m_view = parent;

  addColumn( "" );
  header()->hide();

  m_root = new QListViewItem( this, i18n( "Computer" ) );
  m_root->setSelectable( false );

  connect( this, SIGNAL( doubleClicked( QListViewItem * ) ),
	   this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
  connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
	   this, SLOT( slotRightButtonPressed( QListViewItem * ) ) );
  connect( this, SIGNAL( clicked( QListViewItem * ) ),
	   this, SLOT( slotClicked( QListViewItem * ) ) );

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
  }

}

void KonqDirTree::openSubFolder( KonqDirTreeItem *item, KonqDirTreeItem *topLevel )
{
  qDebug( "openSubFolder( %s )", item->fileItem()->url().url().ascii() );
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  topLevelItem.m_dirLister->openURL( item->fileItem()->url(), false, topLevel ? true : false );
}

void KonqDirTree::addSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const QString &url )
{
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  topLevelItem.m_mapSubDirs->insert( url, item );
}

void KonqDirTree::removeSubDir( KonqDirTreeItem *item, KonqDirTreeItem *topLevel, const QString &url )
{
  TopLevelItem topLevelItem = findTopLevelByItem( topLevel ? topLevel : item );

  assert( topLevelItem.m_item );

  bool bRemoved = topLevelItem.m_mapSubDirs->remove( url );
  assert( bRemoved );
}

void KonqDirTree::slotNewItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  KDirLister *lister = (KDirLister *)sender();

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  KURL dir( item->url() );
  dir.setFileName( "" );

  KonqDirTreeItem *parentDir = findDir( *topLevelItem.m_mapSubDirs, dir.url( 0 ) );

  assert( parentDir );

  KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( this, parentDir, topLevelItem.m_item, item );
  dirTreeItem->setPixmap( 0, item->pixmap( KIconLoader::Small, false ) );
  dirTreeItem->setText( 0, item->url().filename() );
}

void KonqDirTree::slotDeleteItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  qDebug( "slotDeleteItem( %s )", item->url().url().ascii() );

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
      qDebug( "removing %s", item->url().url().ascii() );
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
  {
    unselectAll();
    return;
  }

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

  QList<KonqDirTreeItem> selection = selectedItems();
  QListIterator<KonqDirTreeItem> it( selection );
  for (; it.current(); ++it )
    lstItems.append( it.current()->fileItem() );

  emit m_view->popupMenu( QCursor::pos(), lstItems );
}

void KonqDirTree::slotClicked( QListViewItem *item )
{
  if ( !item )
    unselectAll();
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

  m_unselectableItems.append( item );

  item->setSelectable( false );

  item->setPixmap( 0, KonqFactory::instance()->iconLoader()->loadApplicationIcon( icon, KIconLoader::Small ) );
  item->setText( 0, name );

  KDirLister *dirLister = new KDirLister;
  dirLister->setDirOnlyMode( true );

  connect( dirLister, SIGNAL( newItem( KFileItem * ) ),
	   this, SLOT( slotNewItem( KFileItem * ) ) );
  connect( dirLister, SIGNAL( deleteItem( KFileItem * ) ),
	   this, SLOT( slotDeleteItem( KFileItem * ) ) );

  m_topLevelItems.append( TopLevelItem( item, dirLister, new QDict<KonqDirTreeItem> ) );
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

KonqDirTreeItem *KonqDirTree::findDir( const QDict<KonqDirTreeItem> &dict, const QString &url )
{
  QDictIterator<KonqDirTreeItem> it( dict );
  for (; it.current(); ++it )
    if ( urlcmp( it.current()->fileItem()->url().url( 0 ), url, true, true ) )
      return it.current();

  return 0;
}

QList<KonqDirTreeItem> KonqDirTree::selectedItems()
{
  QList<KonqDirTreeItem> list;

  QListViewItemIterator it( this );
  for (; it.current(); ++it )
    if ( it.current()->isSelected() )
      list.append( (KonqDirTreeItem *)it.current() );

  return list;
}

void KonqDirTree::unselectAll()
{
  QList<KonqDirTreeItem> selection = selectedItems();
  QListIterator<KonqDirTreeItem> it( selection );
  for (; it.current(); ++it )
    it.current()->setSelected( false );
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

#include <konq_dirtree.moc>
