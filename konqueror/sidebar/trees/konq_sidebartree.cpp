/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_sidebartreemodule.h"
#include "konq_sidebartree.h"
#include <qheader.h>
#include <qdir.h>
#include <qfile.h>
#include <qtimer.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirnotify_stub.h>
#include <konq_operations.h>
#include <kglobalsettings.h>
#include <kio/global.h>
#include <kmimetype.h>
#include <kstddirs.h>
#include <kurldrag.h>
#include <stdlib.h>
#include <assert.h>

//#include <dirtree_module/dirtree_module.h> // TEMPORARY HACK
//#include <history_module/history_module.h> // TEMPORARY HACK
//#include <bookmark_module/bookmark_module.h> // TEMPORARY HACK


static const int autoOpenTimeout = 750;


void KonqSidebarTree::loadModuleFactories()
{
  plugins.clear();
  KStandardDirs *dirs=KGlobal::dirs();
  QStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",false,true);
  KLibLoader *loader = KLibLoader::self();

  for (QStringList::ConstIterator it=list.begin();it!=list.end();++it)
        {
				KSimpleConfig ksc(*it);
				ksc.setGroup("Desktop Entry");
				QString name=ksc.readEntry("X-KDE-TreeModule","");
				QString libname=ksc.readEntry("X-KDE-TreeModule-Lib","");
				if ((name.isEmpty()) || (libname.isEmpty()))
					{kdWarning()<<"Bad Configuration file for a dirtree module "<<*it<<endl; continue;}

                                // try to load the library
                                QString lib_name="lib"+libname;
                                KLibrary *lib = loader->library(QFile::encodeName(lib_name));
                                if (lib)
                                {
                                        // get the create_ function
                                        QString factory="create_"+libname;
                                        void *create = lib->symbol(QFile::encodeName(factory));
                                         if (create)
                                                {
							getModule func=(getModule)create;
							plugins.insert(name,func);
							kdDebug()<<"Added a mdoule"<<endl;
                                                }
					 else {kdWarning()<<"No create function found in"<<lib_name<<endl;}
                                }
                                else
                                        kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
	}
}


KonqSidebarTree::KonqSidebarTree( KonqSidebar_Tree *parent, QWidget *parentWidget, int virt, const QString& path )
    : KListView( parentWidget ),
      m_toolTip( this ),
      m_scrollingLocked( false )
{

    loadModuleFactories();

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    m_lstModules.setAutoDelete( true );

    setSelectionMode( QListView::Single );

    m_part = parent;

    m_animationTimer = new QTimer( this );
    connect( m_animationTimer, SIGNAL( timeout() ),
             this, SLOT( slotAnimation() ) );

    m_currentBeforeDropItem = 0;
    m_dropItem = 0;

    addColumn( QString::null );
    header()->hide();

    m_autoOpenTimer = new QTimer( this );
    connect( m_autoOpenTimer, SIGNAL( timeout() ),
             this, SLOT( slotAutoOpenFolder() ) );

    connect( this, SIGNAL( doubleClicked( QListViewItem * ) ),
             this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( this, SIGNAL( mouseButtonPressed(int, QListViewItem*, const QPoint&, int)),
             this, SLOT( slotMouseButtonPressed(int, QListViewItem*, const QPoint&, int)) );
    connect( this, SIGNAL( clicked( QListViewItem * ) ),
             this, SLOT( slotClicked( QListViewItem * ) ) );
    connect( this, SIGNAL( returnPressed( QListViewItem * ) ),
             this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( this, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    connect( this, SIGNAL( onItem( QListViewItem * )),
	     this, SLOT( slotOnItem( QListViewItem * ) ) );
    connect( this, SIGNAL(itemRenamed(QListViewItem*, const QString &, int)),
             this, SLOT(slotItemRenamed(QListViewItem*, const QString &, int)));

    m_bDrag = false;

/*    assert( m_part->getInterfaces()->getInstance()->dirs );
    QString dirtreeDir = m_part->getInterfaces()->getInstance()->dirs()->saveLocation( "data", "konqueror/dirtree/" ); */

//    assert( KGlobal::dirs() );
//    QString dirtreeDir = part->getInterfaces()->getInstance()->dirs()->saveLocation( "data", "konqueror/dirtree/" );

    if (virt==VIRT_Folder) 
		{
		  m_dirtreeDir.dir.setPath(KGlobal::dirs()->saveLocation("data","konqsidebartng/virtual_folders/"+path+"/"));
		  m_dirtreeDir.relDir=path;
		}
	else	
		m_dirtreeDir.dir.setPath( path );
    kdDebug(1201)<<m_dirtreeDir.dir.path()<<endl;
    m_dirtreeDir.type=virt;
    // Initial parsing
    rescanConfiguration();
    if (firstChild()) firstChild()->setOpen(true);
}

KonqSidebarTree::~KonqSidebarTree()
{
    clearTree();
}

void KonqSidebarTree::clearTree()
{
//     for ( KonqSidebarTreeModule * module = m_lstModules.first() ; module ; module = m_lstModules.next() )
//         module->clearAll();
    m_lstModules.clear();
    m_topLevelItems.clear();
    m_mapCurrentOpeningFolders.clear();
    clear();
    setRootIsDecorated( true );
}

void KonqSidebarTree::followURL( const KURL &url )
{
    // Maybe we're there already ?
    KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if (selection && selection->externalURL().cmp( url, true ))
    {
        ensureItemVisible( selection );
        return;
    }

    kdDebug(1201) << "KonqDirTree::followURL: " << url.url() << endl;
    QListIterator<KonqSidebarTreeTopLevelItem> topItem ( m_topLevelItems );
    for (; topItem.current(); ++topItem )
    {
        if ( topItem.current()->externalURL().isParentOf( url ) )
        {
            topItem.current()->module()->followURL( url );
            return; // done
        }
    }
    kdDebug(1201) << "KonqDirTree::followURL: Not found" << endl;
}

void KonqSidebarTree::contentsDragEnterEvent( QDragEnterEvent *ev )
{
    m_dropItem = 0;
    m_currentBeforeDropItem = selectedItem();
    // Save the available formats
    m_lstDropFormats.clear();
    for( int i = 0; ev->format( i ); i++ )
      if ( *( ev->format( i ) ) )
         m_lstDropFormats.append( ev->format( i ) );
}

void KonqSidebarTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    QListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

    // Accept drops on the background, if URLs
    if ( !item && m_lstDropFormats.contains("text/uri-list") )
    {
        m_dropItem = 0;
        e->acceptAction();
        if (selectedItem())
        setSelected( selectedItem(), false ); // no item selected
        return;
    }
    if ( !item || !item->isSelectable() || !static_cast<KonqSidebarTreeItem*>(item)->acceptsDrops( m_lstDropFormats ))
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

void KonqSidebarTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    // Restore the current item to what it was before the dragging (#17070)
    if ( m_currentBeforeDropItem )
        setSelected( m_currentBeforeDropItem, true );
    else
        setSelected( m_dropItem, false ); // no item selected
    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_lstDropFormats.clear();
}

void KonqSidebarTree::contentsDropEvent( QDropEvent *ev )
{
    m_autoOpenTimer->stop();

    if ( !selectedItem() )
    {
//        KonqOperations::doDrop( 0L, m_dirtreeDir.dir, ev, this );
    }
    else
    {
        KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
        selection->drop( ev );
    }
}

void KonqSidebarTree::contentsMousePressEvent( QMouseEvent *e )
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

void KonqSidebarTree::contentsMouseMoveEvent( QMouseEvent *e )
{
    KListView::contentsMouseMoveEvent( e );
    if ( !m_bDrag || ( e->pos() - m_dragPos ).manhattanLength() <= KGlobalSettings::dndEventDelay() )
        return;

    m_bDrag = false;

    QListViewItem *item = itemAt( contentsToViewport( m_dragPos ) );

    if ( !item || !item->isSelectable() )
        return;

    // Start a drag
    QDragObject * drag = static_cast<KonqSidebarTreeItem *>( item )->dragObject( viewport(), false );
    if ( !drag )
	return;

    const QPixmap *pix = item->pixmap(0);
    if ( pix && drag->pixmap().isNull() ) {
        QPoint hotspot( pix->width() / 2, pix->height() / 2 );
        drag->setPixmap( *pix, hotspot );
    }

    drag->drag();
}

void KonqSidebarTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
    KListView::contentsMouseReleaseEvent( e );
    m_bDrag = false;
}

void KonqSidebarTree::leaveEvent( QEvent *e )
{
    KListView::leaveEvent( e );
    m_part->emitStatusBarText( QString::null );
}


void KonqSidebarTree::slotDoubleClicked( QListViewItem *item )
{
    //kdDebug(1201) << "KonqSidebarTree::slotDoubleClicked " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    slotClicked( item );
    item->setOpen( !item->isOpen() );
}

void KonqSidebarTree::slotClicked( QListViewItem *item )
{
    //kdDebug(1201) << "KonqSidebarTree::slotClicked " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    KonqSidebarTreeItem *dItem = static_cast<KonqSidebarTreeItem *>( item );

    KParts::URLArgs args;

    args.serviceType = dItem->externalMimeType();
    args.trustedSource = true;
    emit m_part->getInterfaces()->getExtension()->openURLRequest( dItem->externalURL(), args );
}

void KonqSidebarTree::slotMouseButtonPressed(int _button, QListViewItem* _item, const QPoint&, int col)
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>(_item);
    if(_item && col < 2)
        if (_button == MidButton)
            item->middleButtonPressed();
        else if (_button == RightButton)
        {
            item->setSelected( true );
            item->rightButtonPressed();
        }
}

void KonqSidebarTree::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

    if ( !m_dropItem || m_dropItem->isOpen() )
        return;

    m_dropItem->setOpen( true );
    m_dropItem->repaint();
}

void KonqSidebarTree::rescanConfiguration()
{
    kdDebug(1201) << "KonqSidebarTree::rescanConfiguration()" << endl;
    m_autoOpenTimer->stop();
    clearTree();
    if (m_dirtreeDir.type==VIRT_Folder)
	{
	         kdDebug(1201)<<"KonqSidebarTree::rescanConfiguration()-->scanDir"<<endl;
		 scanDir( 0, m_dirtreeDir.dir.path(), true);

	}
	else
		{
			    kdDebug(1201)<<"KonqSidebarTree::rescanConfiguration()-->loadTopLevel"<<endl;
		            loadTopLevelItem( 0, m_dirtreeDir.dir.path() ); 
		}
}

void KonqSidebarTree::slotSelectionChanged()
{
    if ( !m_dropItem ) // don't do this while the dragmove thing
    {
        KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
        if ( item )
            item->itemSelected();
        /* else   -- doesn't seem to happen
           {} */
    }
}

void KonqSidebarTree::FilesAdded( const KURL & dir )
{
    kdDebug(1201) << "KonqSidebarTree::FilesAdded " << dir.url() << endl;
    if ( m_dirtreeDir.dir.isParentOf( dir ) )
        // We use a timer in case of DCOP re-entrance..
        QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
}

void KonqSidebarTree::FilesRemoved( const KURL::List & urls )
{
    //kdDebug(1201) << "KonqSidebarTree::FilesRemoved " << urls.count() << endl;
    for ( KURL::List::ConstIterator it = urls.begin() ; it != urls.end() ; ++it )
    {
        //kdDebug(1201) <<  "KonqSidebarTree::FilesRemoved " << (*it).prettyURL() << endl;
        if ( m_dirtreeDir.dir.isParentOf( *it ) )
        {
            QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
            kdDebug(1201) << "KonqSidebarTree::FilesRemoved done" << endl;
            return;
        }
    }
}

void KonqSidebarTree::FilesChanged( const KURL::List & urls )
{
    //kdDebug(1201) << "KonqSidebarTree::FilesChanged" << endl;
    // not same signal, but same implementation
    FilesRemoved( urls );
}

void KonqSidebarTree::scanDir( KonqSidebarTreeItem *parent, const QString &path, bool isRoot )
{
    QDir dir( path );

    if ( !dir.isReadable() )
        return;

    kdDebug(1201) << "scanDir " << path << endl;

    QStringList entries = dir.entryList( QDir::Files );
    QStringList dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks );
    dirEntries.remove( "." );
    dirEntries.remove( ".." );

    if ( isRoot )
    {
        bool copyConfig = ( entries.count() == 0 && dirEntries.count() == 0 );
        if (!copyConfig)
        {
            // Check version number
            // Version 1 was the dirtree of KDE 2.0.x (no versioning at that time, so default)
            // Version 2 includes the history
            // Version 3 includes the bookmarks
            // Version 4 includes lan.desktop and floppy.desktop, Alex
            // Version 5 includes some stuff from Rob Kaper
            const int currentVersion = 5;
            QString key = QString::fromLatin1("X-KDE-DirTreeVersionNumber");
            KSimpleConfig versionCfg( path + "/.directory" );
            int versionNumber = versionCfg.readNumEntry( key, 1 );
            kdDebug(1201) << "KonqSidebarTree::scanDir found version " << versionNumber << endl;
            if ( versionNumber < currentVersion )
            {
                versionCfg.writeEntry( key, currentVersion );
                versionCfg.sync();
                copyConfig = true;
            }
        }
        if (copyConfig)
        {
            // We will copy over the configuration for the dirtree, from the global directory
            QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/").last();  // most global
            kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir=" << dirtree_dir << endl;

            /*
            // debug code

            QStringList blah = m_part->getInterfaces->getInstance()->dirs()->dirs()->findDirs( "data", "konqueror/dirtree" );
            QStringList::ConstIterator eIt = blah.begin();
            QStringList::ConstIterator eEnd = blah.end();
            for (; eIt != eEnd; ++eIt )
                kdDebug(1201) << "KonqSidebarTree::scanDir findDirs got me " << *eIt << endl;
            // end debug code
            */

            if ( !dirtree_dir.isEmpty() && dirtree_dir != path )
            {
                QDir globalDir( dirtree_dir );
                ASSERT( globalDir.isReadable() );
                // Only copy the entries that don't exist yet in the local dir
                QStringList globalDirEntries = globalDir.entryList();
                QStringList::ConstIterator eIt = globalDirEntries.begin();
                QStringList::ConstIterator eEnd = globalDirEntries.end();
                for (; eIt != eEnd; ++eIt )
                {
                    //kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir contains " << *eIt << endl;
                    if ( *eIt != "." && *eIt != ".."
                         && !entries.contains( *eIt ) && !dirEntries.contains( *eIt ) )
                    { // we don't have that one yet -> copy it.
                        QString cp = QString("cp -R %1%2 %3").arg(dirtree_dir).arg(*eIt).arg(path);
                        kdDebug(1201) << "KonqSidebarTree::scanDir executing " << cp << endl;
                        ::system( cp.local8Bit().data() );
                    }
                }

                // hack to make QDir refresh the lists
                dir.setPath(path);
                entries = dir.entryList( QDir::Files );
                dirEntries = dir.entryList( QDir::Dirs );
                dirEntries.remove( "." );
                dirEntries.remove( ".." );
            }
        }
    }

    QStringList::ConstIterator eIt = entries.begin();
    QStringList::ConstIterator eEnd = entries.end();

    for (; eIt != eEnd; ++eIt )
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

        loadTopLevelGroup( parent, newPath );
    }
}

void KonqSidebarTree::loadTopLevelGroup( KonqSidebarTreeItem *parent, const QString &path )
{
    QDir dir( path );
    QString name = dir.dirName();
    QString icon = "folder";
    bool    open = false;

    kdDebug(1201) << "Scanning " << path << endl;

    QString dotDirectoryFile = QString( path ).append( "/.directory" );

    if ( QFile::exists( dotDirectoryFile ) )
    {
        kdDebug(1201) << "Reading the .directory" << endl;
        KSimpleConfig cfg( dotDirectoryFile, true );
        cfg.setDesktopGroup();
        name = cfg.readEntry( "Name", name );
        icon = cfg.readEntry( "Icon", icon );
        //stripIcon( icon );
        open = cfg.readBoolEntry( "Open", open );
    }

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
    {
        kdDebug(1201) << "KonqSidebarTree::loadTopLevelGroup Inserting new group under parent " << endl;
        item = new KonqSidebarTreeTopLevelItem( parent, 0 /* no module */, path );
    }
    else
        item = new KonqSidebarTreeTopLevelItem( this, 0 /* no module */, path );
    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( icon ) );
    item->setListable( false );
    item->setClickable( false );
    item->setTopLevelGroup( true );
    item->setOpen( open );

    m_topLevelItems.append( item );

    kdDebug(1201) << "Inserting group " << name << "   " << path << endl;;

    scanDir( item, path );

    if ( item->childCount() == 0 )
        item->setExpandable( false );
}

void KonqSidebarTree::loadTopLevelItem( KonqSidebarTreeItem *parent,  const QString &filename )
{
    KDesktopFile cfg( filename, true );
    cfg.setDollarExpansion(true);

    QFileInfo inf( filename );

    QString path = filename;
    QString name = KIO::decodeFileName( inf.fileName() );
    if ( name.length() > 8 && name.right( 8 ) == ".desktop" )
        name.truncate( name.length() - 8 );
    if ( name.length() > 7 && name.right( 7 ) == ".kdelnk" )
        name.truncate( name.length() - 7 );

    name = cfg.readEntry( "Name", name );
    KonqSidebarTreeModule * module = 0L;

  module=0;

    // Here's where we need to create the right module...
    // ### TODO: make this KTrader/KLibrary based.
    QString moduleName = cfg.readEntry( "X-KDE-TreeModule" );
    kdDebug(1201) << "##### Loading module: " << moduleName << " file: " << filename << endl;

    getModule func;
    func=plugins[moduleName];
    if (func!=0)
	{
		module=func(this);
	}

    if (module==0) {kdDebug()<<"No Module loaded"<<endl; return;}

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
        item = new KonqSidebarTreeTopLevelItem( parent, module, path );
    else
        item = new KonqSidebarTreeTopLevelItem( this, module, path );

    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( cfg.readIcon() ));

    module->addTopLevelItem( item );

    m_topLevelItems.append( item );
    m_lstModules.append( module );

    bool open = cfg.readBoolEntry( "Open", false );
    if ( open && item->isExpandable() )
        item->setOpen( true );
}

void KonqSidebarTree::slotAnimation()
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.begin();
    MapCurrentOpeningFolders::Iterator end = m_mapCurrentOpeningFolders.end();
    for (; it != end; ++it )
    {
        uint & iconNumber = it.data().iconNumber;
        QString icon = QString::fromLatin1( it.data().iconBaseName ).append( QString::number( iconNumber ) );
        it.key()->setPixmap( 0, SmallIcon( icon));

        iconNumber++;
        if ( iconNumber > it.data().iconCount )
            iconNumber = 1;
    }
}


void KonqSidebarTree::startAnimation( KonqSidebarTreeItem * item, const char * iconBaseName, uint iconCount )
{
    m_mapCurrentOpeningFolders.insert( item, AnimationInfo( iconBaseName, iconCount, *item->pixmap(0) ) );
    if ( !m_animationTimer->isActive() )
        m_animationTimer->start( 50 );
}

void KonqSidebarTree::stopAnimation( KonqSidebarTreeItem * item )
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.find(item);
    if ( it != m_mapCurrentOpeningFolders.end() )
    {
        item->setPixmap( 0, it.data().originalPixmap );
        m_mapCurrentOpeningFolders.remove( item );
    }
    if (m_mapCurrentOpeningFolders.isEmpty())
        m_animationTimer->stop();
}

KonqSidebarTreeItem * KonqSidebarTree::currentItem() const
{
    return static_cast<KonqSidebarTreeItem *>( selectedItem() );
}

void KonqSidebarTree::slotOnItem( QListViewItem *item )
{
    KonqSidebarTreeItem *i = static_cast<KonqSidebarTreeItem *>( item );
    const KURL& url = i->externalURL();
    if ( url.isLocalFile() )
	m_part->emitStatusBarText( url.path() );
    else
	m_part->emitStatusBarText( url.prettyURL() );
}

void KonqSidebarTree::setContentsPos( int x, int y )
{
    if ( !m_scrollingLocked )
	KListView::setContentsPos( x, y );
}

void KonqSidebarTree::slotItemRenamed(QListViewItem* item, const QString &name, int col)
{
    ASSERT(col==0);
    if (col != 0) return;
    assert(item);
    KonqSidebarTreeItem * treeItem = static_cast<KonqSidebarTreeItem *>(item);
    if ( treeItem->isTopLevelItem() )
    {
        KonqSidebarTreeTopLevelItem * topLevelItem = static_cast<KonqSidebarTreeTopLevelItem *>(treeItem);
        topLevelItem->rename( name );
    }
    else
        kdWarning() << "slotItemRenamed: rename not implemented for non-toplevel items" << endl;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


void KonqSidebarTreeToolTip::maybeTip( const QPoint &point )
{
    QListViewItem *item = m_view->itemAt( point );
    if ( item ) {
	QString text = static_cast<KonqSidebarTreeItem*>( item )->toolTipText();
	if ( !text.isEmpty() )
	    tip ( m_view->itemRect( item ), text );
    }
}

#include "konq_sidebartree.moc"
