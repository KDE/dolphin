/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                 2003 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_sidebartreemodule.h"

#include <qclipboard.h>
#include <qcursor.h>
#include <qdir.h>
#include <q3header.h>
#include <q3popupmenu.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QPixmap>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QEvent>
#include <Q3Frame>
#include <QDropEvent>
#include <QDragEnterEvent>

#include <dcopclient.h>
#include <dcopref.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirnotify_stub.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <kmimetype.h>
#include <kprocess.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <k3urldrag.h>

#include <stdlib.h>
#include <assert.h>


static const int autoOpenTimeout = 750;


getModule KonqSidebarTree::getPluginFactory(QString name)
{
  if (!pluginFactories.contains(name))
  {
    KLibLoader *loader = KLibLoader::self();
    QString libName    = pluginInfo[name];
    KLibrary *lib      = loader->library(QFile::encodeName(libName));
    if (lib)
    {
      // get the create_ function
      QString factory = "create_" + libName;
      void *create    = lib->symbol(QFile::encodeName(factory));
      if (create)
      {
        getModule func = (getModule)create;
        pluginFactories.insert(name, func);
        kdDebug()<<"Added a module"<<endl;
      }
      else
      {
        kdWarning()<<"No create function found in"<<libName<<endl;
      }
    }
    else
      kdWarning() << "Module " << libName << " can't be loaded!" << endl;
  }

  return pluginFactories[name];
}

void KonqSidebarTree::loadModuleFactories()
{
  pluginFactories.clear();
  pluginInfo.clear();
  KStandardDirs *dirs=KGlobal::dirs();
  QStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",false,true);


  for (QStringList::ConstIterator it=list.begin();it!=list.end();++it)
  {
    KSimpleConfig ksc(*it);
    ksc.setGroup("Desktop Entry");
    QString name    = ksc.readEntry("X-KDE-TreeModule");
    QString libName = ksc.readEntry("X-KDE-TreeModule-Lib");
    if ((name.isEmpty()) || (libName.isEmpty()))
        {kdWarning()<<"Bad Configuration file for a dirtree module "<<*it<<endl; continue;}

    //Register the library info.
    pluginInfo[name] = libName;
  }
}


class KonqSidebarTree_Internal
{
public:
    DropAcceptType m_dropMode;
    QStringList m_dropFormats;
};


KonqSidebarTree::KonqSidebarTree( KonqSidebar_Tree *parent, QWidget *parentWidget, int virt, const QString& path )
    : KListView( parentWidget ),
      m_currentTopLevelItem( 0 ),
      m_scrollingLocked( false ),
      m_collection( 0 )
{
    d = new KonqSidebarTree_Internal;
    d->m_dropMode = SidebarTreeMode;

    loadModuleFactories();

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    m_lstModules.setAutoDelete( true );

    setSelectionMode( Q3ListView::Single );
    setDragEnabled(true);

    m_part = parent;

    m_animationTimer = new QTimer( this );
    connect( m_animationTimer, SIGNAL( timeout() ),
             this, SLOT( slotAnimation() ) );

    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_bOpeningFirstChild=false;

    addColumn( QString::null );
    header()->hide();
    setTreeStepSize(15);

    m_autoOpenTimer = new QTimer( this );
    connect( m_autoOpenTimer, SIGNAL( timeout() ),
             this, SLOT( slotAutoOpenFolder() ) );

    connect( this, SIGNAL( doubleClicked( Q3ListViewItem * ) ),
             this, SLOT( slotDoubleClicked( Q3ListViewItem * ) ) );
    connect( this, SIGNAL( mouseButtonPressed(int, Q3ListViewItem*, const QPoint&, int)),
             this, SLOT( slotMouseButtonPressed(int, Q3ListViewItem*, const QPoint&, int)) );
    connect( this, SIGNAL( mouseButtonClicked( int, Q3ListViewItem*, const QPoint&, int ) ),
	     this, SLOT( slotMouseButtonClicked( int, Q3ListViewItem*, const QPoint&, int ) ) );
    connect( this, SIGNAL( returnPressed( Q3ListViewItem * ) ),
             this, SLOT( slotDoubleClicked( Q3ListViewItem * ) ) );
    connect( this, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    connect( this, SIGNAL(itemRenamed(Q3ListViewItem*, const QString &, int)),
             this, SLOT(slotItemRenamed(Q3ListViewItem*, const QString &, int)));

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

    if (firstChild())
    {
      m_bOpeningFirstChild = true;
      firstChild()->setOpen(true);
      m_bOpeningFirstChild = false;
    }

    setFrameStyle( Q3Frame::ToolBarPanel | Q3Frame::Raised );
}

KonqSidebarTree::~KonqSidebarTree()
{
    clearTree();

    delete d;
}

void KonqSidebarTree::itemDestructed( KonqSidebarTreeItem *item )
{
    stopAnimation(item);

    if (item == m_currentBeforeDropItem)
    {
       m_currentBeforeDropItem = 0;
    }
}

void KonqSidebarTree::setDropFormats(const QStringList &formats)
{
    d->m_dropFormats = formats;
}

void KonqSidebarTree::clearTree()
{
    m_lstModules.clear();
    m_topLevelItems.clear();
    m_mapCurrentOpeningFolders.clear();
    m_currentBeforeDropItem = 0;
    clear();

    if (m_dirtreeDir.type==VIRT_Folder)
    {
        setRootIsDecorated( true );
    }
    else
    {
        setRootIsDecorated( false );
    }
}

void KonqSidebarTree::followURL( const KURL &url )
{
    // Maybe we're there already ?
    KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if (selection && selection->externalURL().equals( url, true ))
    {
        ensureItemVisible( selection );
        return;
    }

    kdDebug(1201) << "KonqDirTree::followURL: " << url.url() << endl;
    Q3PtrListIterator<KonqSidebarTreeTopLevelItem> topItem ( m_topLevelItems );
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
    Q3ListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

    // Accept drops on the background, if URLs
    if ( !item && m_lstDropFormats.contains("text/uri-list") )
    {
        m_dropItem = 0;
        e->acceptAction();
        if (selectedItem())
        setSelected( selectedItem(), false ); // no item selected
        return;
    }

    if (item && static_cast<KonqSidebarTreeItem*>(item)->acceptsDrops( m_lstDropFormats )) {
        d->m_dropMode = SidebarTreeMode;

        if ( !item->isSelectable() )
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
    } else {
        d->m_dropMode = KListViewMode;
        KListView::contentsDragMoveEvent(e);
    }
}

void KonqSidebarTree::contentsDragLeaveEvent( QDragLeaveEvent *ev )
{
    // Restore the current item to what it was before the dragging (#17070)
    if ( m_currentBeforeDropItem )
        setSelected( m_currentBeforeDropItem, true );
    else
        setSelected( m_dropItem, false ); // no item selected
    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_lstDropFormats.clear();

    if (d->m_dropMode == KListViewMode) {
        KListView::contentsDragLeaveEvent(ev);
    }
}

void KonqSidebarTree::contentsDropEvent( QDropEvent *ev )
{
    if (d->m_dropMode == SidebarTreeMode) {
        m_autoOpenTimer->stop();

        if ( !selectedItem() )
        {
    //        KonqOperations::doDrop( 0L, m_dirtreeDir.dir, ev, this );
            KURL::List urls;
            if ( K3URLDrag::decode( ev, urls ) )
            {
               for(KURL::List::ConstIterator it = urls.begin();
                   it != urls.end(); ++it)
               {
                  addURL(0, *it);
               }
            }
        }
        else
        {
            KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
            selection->drop( ev );
        }
    } else {
        KListView::contentsDropEvent(ev);
    }
}

static QString findUniqueFilename(const QString &path, QString filename)
{
    if (filename.endsWith(".desktop"))
       filename.truncate(filename.length()-8);

    QString name = filename;
    int n = 2;
    while(QFile::exists(path + filename + ".desktop"))
    {
       filename = QString("%2_%1").arg(n++).arg(name);
    }
    return path+filename+".desktop";
}

void KonqSidebarTree::addURL(KonqSidebarTreeTopLevelItem* item, const KURL & url)
{
    QString path;
    if (item)
       path = item->path();
    else
       path = m_dirtreeDir.dir.path();

    KURL destUrl;

    if (url.isLocalFile() && url.fileName().endsWith(".desktop"))
    {
       QString filename = findUniqueFilename(path, url.fileName());
       destUrl.setPath(filename);
       KIO::NetAccess::copy(url, destUrl, this);
    }
    else
    {
       QString name = url.host();
       if (name.isEmpty())
          name = url.fileName();
       QString filename = findUniqueFilename(path, name);
       destUrl.setPath(filename);

       KDesktopFile cfg(filename);
       cfg.writeEntry("Encoding", "UTF-8");
       cfg.writeEntry("Type","Link");
       cfg.writeEntry("URL", url.url());
       QString icon = "folder";
       if (!url.isLocalFile())
          icon = KMimeType::favIconForURL(url);
       if (icon.isEmpty())
          icon = KProtocolInfo::icon( url.protocol() );
       cfg.writeEntry("Icon", icon);
       cfg.writeEntry("Name", name);
       cfg.writeEntry("Open", false);
       cfg.sync();
    }

    KDirNotify_stub allDirNotify( "*", "KDirNotify*" );
    destUrl.setPath( destUrl.directory() );
    allDirNotify.FilesAdded( destUrl );

    if (item)
       item->setOpen(true);
}

bool KonqSidebarTree::acceptDrag(QDropEvent* e) const
{
    // for KListViewMode...
    for( int i = 0; e->format( i ); i++ )
        if ( d->m_dropFormats.contains(e->format( i ) ) )
            return true;
    return false;
}

Q3DragObject* KonqSidebarTree::dragObject()
{
    KonqSidebarTreeItem* item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if ( !item )
        return 0;

    Q3DragObject* drag = item->dragObject( viewport(), false );
    if ( !drag )
        return 0;

    const QPixmap *pix = item->pixmap(0);
    if ( pix && drag->pixmap().isNull() )
        drag->setPixmap( *pix );

    return drag;
}

void KonqSidebarTree::leaveEvent( QEvent *e )
{
    KListView::leaveEvent( e );
//    emitStatusBarText( QString::null );
}


void KonqSidebarTree::slotDoubleClicked( Q3ListViewItem *item )
{
    //kdDebug(1201) << "KonqSidebarTree::slotDoubleClicked " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    slotExecuted( item );
    item->setOpen( !item->isOpen() );
}

void KonqSidebarTree::slotExecuted( Q3ListViewItem *item )
{
    kdDebug(1201) << "KonqSidebarTree::slotExecuted " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    KonqSidebarTreeItem *dItem = static_cast<KonqSidebarTreeItem *>( item );

    KParts::URLArgs args;

    args.serviceType = dItem->externalMimeType();
    args.trustedSource = true;
    KURL externalURL = dItem->externalURL();
    if ( !externalURL.isEmpty() )
	openURLRequest( externalURL, args );
}

void KonqSidebarTree::slotMouseButtonPressed( int _button, Q3ListViewItem* _item, const QPoint&, int col )
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>( _item );
    if (_button == Qt::RightButton)
    {
        if ( item && col < 2)
        {
            item->setSelected( true );
            item->rightButtonPressed();
        }
    }
}

void KonqSidebarTree::slotMouseButtonClicked(int _button, Q3ListViewItem* _item, const QPoint&, int col)
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>(_item);
    if(_item && col < 2)
    {
        switch( _button ) {
        case Qt::LeftButton:
            slotExecuted( item );
            break;
        case Qt::MidButton:
            item->middleButtonClicked();
            break;
        }
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
            // Version 5 includes the audiocd browser
            // Version 6 includes the printmanager and lan browser
            const int currentVersion = 6;
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
            QStringList dirtree_dirs = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/");


//            QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/").last();  // most global
//            kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir=" << dirtree_dir << endl;

            /*
            // debug code

            QStringList blah = m_part->getInterfaces->getInstance()->dirs()->dirs()->findDirs( "data", "konqueror/dirtree" );
            QStringList::ConstIterator eIt = blah.begin();
            QStringList::ConstIterator eEnd = blah.end();
            for (; eIt != eEnd; ++eIt )
                kdDebug(1201) << "KonqSidebarTree::scanDir findDirs got me " << *eIt << endl;
            // end debug code
            */

	    for (QStringList::const_iterator ddit=dirtree_dirs.begin();ddit!=dirtree_dirs.end();++ddit) {
		QString dirtree_dir=*ddit;
		if (dirtree_dir==path) continue;
	        //    if ( !dirtree_dir.isEmpty() && dirtree_dir != path )
	            {
        	        QDir globalDir( dirtree_dir );
                	Q_ASSERT( globalDir.isReadable() );
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
                	        QString cp("cp -R -- ");
        	                cp += KProcess::quote(dirtree_dir + *eIt);
	                        cp += " ";
        	                cp += KProcess::quote(path);
                	        kdDebug(1201) << "KonqSidebarTree::scanDir executing " << cp << endl;
                        	::system( QFile::encodeName(cp) );
	                    }
        	        }
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
        QString newPath = QString( path ).append( *eIt ).append( QLatin1Char( '/' ) );

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

    kdDebug(1201) << "Inserting group " << name << "   " << path << endl;

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

    // Here's where we need to create the right module...
    // ### TODO: make this KTrader/KLibrary based.
    QString moduleName = cfg.readEntry( "X-KDE-TreeModule" );
    QString showHidden=cfg.readEntry("X-KDE-TreeModule-ShowHidden");

    if (moduleName.isEmpty()) moduleName="Directory";
    kdDebug(1201) << "##### Loading module: " << moduleName << " file: " << filename << endl;

    getModule func;
    func = getPluginFactory(moduleName);
    if (func!=0)
	{
		kdDebug(1201)<<"showHidden: "<<showHidden<<endl;
		module=func(this,showHidden.toUpper()=="TRUE");
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


void KonqSidebarTree::startAnimation( KonqSidebarTreeItem * item, const char * iconBaseName, uint iconCount, const QPixmap * originalPixmap )
{
    const QPixmap *pix = originalPixmap ? originalPixmap : item->pixmap(0);
    if (pix)
    {
        m_mapCurrentOpeningFolders.insert( item, AnimationInfo( iconBaseName, iconCount, *pix ) );
        if ( !m_animationTimer->isActive() )
            m_animationTimer->start( 50 );
    }
}

void KonqSidebarTree::stopAnimation( KonqSidebarTreeItem * item )
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.find(item);
    if ( it != m_mapCurrentOpeningFolders.end() )
    {
        item->setPixmap( 0, it.data().originalPixmap );
        m_mapCurrentOpeningFolders.remove( item );

        if (m_mapCurrentOpeningFolders.isEmpty())
            m_animationTimer->stop();
    }
}

KonqSidebarTreeItem * KonqSidebarTree::currentItem() const
{
    return static_cast<KonqSidebarTreeItem *>( selectedItem() );
}

void KonqSidebarTree::setContentsPos( int x, int y )
{
    if ( !m_scrollingLocked )
	KListView::setContentsPos( x, y );
}

void KonqSidebarTree::slotItemRenamed(Q3ListViewItem* item, const QString &name, int col)
{
    Q_ASSERT(col==0);
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


void KonqSidebarTree::enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool rename)
{
    enableAction( "copy", copy );
    enableAction( "cut", cut );
    enableAction( "paste", paste );
    enableAction( "trash", trash );
    enableAction( "del", del );
    enableAction( "rename", rename );
}

bool KonqSidebarTree::tabSupport()
{
    // see if the newTab() dcop function is available (i.e. the sidebar is embedded into konqueror)
   DCOPRef ref(kapp->dcopClient()->appId(), topLevelWidget()->name());
    DCOPReply reply = ref.call("functions()");
    if (reply.isValid()) {
        Q3StrList funcs;
        reply.get(funcs, "QCStringList");
        for (Q3StrList::ConstIterator it = funcs.begin(); it != funcs.end(); ++it) {
            if ((*it) == "void newTab(QString url)") {
                return true;
                break;
            }
        }
    }
    return false;
}

void KonqSidebarTree::showToplevelContextMenu()
{
    KonqSidebarTreeTopLevelItem *item = 0;
    KonqSidebarTreeItem *treeItem = currentItem();
    if (treeItem && treeItem->isTopLevelItem())
        item = static_cast<KonqSidebarTreeTopLevelItem *>(treeItem);

    if (!m_collection)
    {
        m_collection = new KActionCollection( this);
	    m_collection->setObjectName("bookmark actions");
        (void) new KAction( i18n("&Create New Folder..."), "folder_new", 0, this,
                            SLOT( slotCreateFolder() ), m_collection, "create_folder");
        (void) new KAction( i18n("Delete Folder"), "editdelete", 0, this,
                            SLOT( slotDelete() ), m_collection, "delete_folder");
        (void) new KAction( i18n("Rename"), 0, this,
                            SLOT( slotRename() ), m_collection, "rename");
        (void) new KAction( i18n("Delete Link"), "editdelete", 0, this,
                            SLOT( slotDelete() ), m_collection, "delete_link");
        (void) new KAction( i18n("Properties"), "edit", 0, this,
                            SLOT( slotProperties() ), m_collection, "item_properties");
        (void) new KAction( i18n("Open in New Window"), "window_new", 0, this,
                            SLOT( slotOpenNewWindow() ), m_collection, "open_window");
        (void) new KAction( i18n("Open in New Tab"), "tab_new", 0, this,
                            SLOT( slotOpenTab() ), m_collection, "open_tab");
        (void) new KAction( i18n("Copy Link Address"), "editcopy", 0, this,
                            SLOT( slotCopyLocation() ), m_collection, "copy_location");
    }

    Q3PopupMenu *menu = new Q3PopupMenu;

    if (item) {
        if (item->isTopLevelGroup()) {
            m_collection->action("rename")->plug(menu);
            m_collection->action("delete_folder")->plug(menu);
            menu->insertSeparator();
            m_collection->action("create_folder")->plug(menu);
        } else {
            if (tabSupport())
                m_collection->action("open_tab")->plug(menu);
            m_collection->action("open_window")->plug(menu);
            m_collection->action("copy_location")->plug(menu);
            menu->insertSeparator();
            m_collection->action("rename")->plug(menu);
            m_collection->action("delete_link")->plug(menu);
        }
        menu->insertSeparator();
        m_collection->action("item_properties")->plug(menu);
    } else {
        m_collection->action("create_folder")->plug(menu);
    }

    m_currentTopLevelItem = item;

    menu->exec( QCursor::pos() );
    delete menu;

    m_currentTopLevelItem = 0;
}

void KonqSidebarTree::slotCreateFolder()
{
    QString path;
    QString name = i18n("New Folder");

    while(true)
    {
        name = KInputDialog::getText(i18n("Create New Folder"),
    			i18n("Enter folder name:"), name);
        if (name.isEmpty())
            return;

        if (m_currentTopLevelItem)
            path = m_currentTopLevelItem->path();
        else
            path = m_dirtreeDir.dir.path();

        if (!path.endsWith("/"))
            path += "/";

        path = path + name;

        if (!QFile::exists(path))
            break;

        name = name + "-2";
   }

   KGlobal::dirs()->makeDir(path);

   loadTopLevelGroup(m_currentTopLevelItem, path);
}

void KonqSidebarTree::slotDelete()
{
    if (!m_currentTopLevelItem) return;
    m_currentTopLevelItem->del();
}

void KonqSidebarTree::slotRename()
{
    if (!m_currentTopLevelItem) return;
    m_currentTopLevelItem->rename();
}

void KonqSidebarTree::slotProperties()
{
    if (!m_currentTopLevelItem) return;

    KURL url;
    url.setPath(m_currentTopLevelItem->path());

    KPropertiesDialog *dlg = new KPropertiesDialog( url );
    dlg->setFileNameReadOnly(true);
    dlg->exec();
    delete dlg;
}

void KonqSidebarTree::slotOpenNewWindow()
{
    if (!m_currentTopLevelItem) return;
    emit createNewWindow( m_currentTopLevelItem->externalURL() );
}

void KonqSidebarTree::slotOpenTab()
{
    if (!m_currentTopLevelItem) return;
    DCOPRef ref(kapp->dcopClient()->appId(), topLevelWidget()->name());   
    ref.call( "newTab(QString)", m_currentTopLevelItem->externalURL().url() );
}

void KonqSidebarTree::slotCopyLocation()
{
    if (!m_currentTopLevelItem) return;
    KURL url = m_currentTopLevelItem->externalURL();
    kapp->clipboard()->setData( new K3URLDrag(url, 0), QClipboard::Selection );
    kapp->clipboard()->setData( new K3URLDrag(url, 0), QClipboard::Clipboard );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#warning KonqSidebarTreeToolTip removed, must implemented in event() function
/*
void KonqSidebarTreeToolTip::maybeTip( const QPoint &point )
{
    Q3ListViewItem *item = m_view->itemAt( point );
    if ( item ) {
	QString text = static_cast<KonqSidebarTreeItem*>( item )->toolTipText();
	if ( !text.isEmpty() )
	    tip ( m_view->itemRect( item ), text );
    }
}
*/



#include "konq_sidebartree.moc"
