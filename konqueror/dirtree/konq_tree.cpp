/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "konq_treepart.h"
#include "konq_tree.h"
#include <qheader.h>
#include <qtimer.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <kurldrag.h>
#include <kprotocolinfo.h>
#include <kdirnotify_stub.h>
#include <kmimetype.h>
#include <assert.h>

static const int autoOpenTimeout = 750;

KonqTreeModule * KonqTree::currentModule() const
{
    KonqTreeItem * item = static_cast<KonqTreeItem *>( selectedItem() );
    ASSERT(item);
    return item ? item->topLevelItem()->module() : 0;
}

KonqTree::KonqTree( KonqTreePart *parent, QWidget *parentWidget )
    : KListView( parentWidget )
{
    m_folderPixmap = KMimeType::mimeType( "inode/directory" )->pixmap( KIcon::Desktop, KIcon::SizeSmall );

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );

    setSelectionMode( QListView::Single );

    m_part = parent;

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
    connect( this, SIGNAL( mouseButtonPressed(int, QListViewItem*, const QPoint&, int)),
             this, SLOT( slotMouseButtonPressed(int, QListViewItem*, const QPoint&, int)) );
    connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             this, SLOT( slotRightButtonPressed( QListViewItem * ) ) );
    connect( this, SIGNAL( clicked( QListViewItem * ) ),
             this, SLOT( slotClicked( QListViewItem * ) ) );
    connect( this, SIGNAL( returnPressed( QListViewItem * ) ),
             this, SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( this, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    m_bDrag = false;

    assert( KonqTreeFactory::instance()->dirs() );
    QString dirtreeDir = KonqTreeFactory::instance()->dirs()->saveLocation( "data", "konqueror/dirtree/" );
    m_dirtreeDir.setPath( dirtreeDir );

    // Initial parsing
    rescanConfiguration();
}

KonqTree::~KonqTree()
{
    clearTree();
}

void KonqTree::clearTree()
{
    for ( KonqTreeModule * module = m_lstModules.first() ; module ; module = m_lstModules.next() )
        module->clearAll();
    m_topLevelItems.clear();
    m_mapCurrentOpeningFolders.clear();
    clear();
    setRootIsDecorated( true );
}

void KonqTree::followURL( const KURL &_url )
{
#if 0 // TODO - move to KonqDirTreeModule and find the right module to call ( ???????? )
    // Maybe we're there already ?
    KonqTreeItem *selection = static_cast<KonqTreeItem *>( selectedItem() );
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
#endif
}

void KonqTree::contentsDragEnterEvent( QDragEnterEvent *ev )
{
    m_dropItem = 0;
    // Save the available formats
    m_lstDropFormats.clear();
    for( int i = 0; ev->format( i ); i++ )
      if ( *( ev->format( i ) ) )
         m_lstDropFormats.append( ev->format( i ) );
}

void KonqTree::contentsDragMoveEvent( QDragMoveEvent *e )
{
    QListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

    if ( !item || !item->isSelectable() || !static_cast<KonqTreeItem*>(item)->acceptsDrops( m_lstDropFormats ))
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

void KonqTree::contentsDragLeaveEvent( QDragLeaveEvent * )
{
    m_dropItem = 0;
    m_lstDropFormats.clear();
}

void KonqTree::contentsDropEvent( QDropEvent *ev )
{
    m_autoOpenTimer->stop();

    KonqTreeItem *selection = static_cast<KonqTreeItem *>( selectedItem() );

    assert( selection );

    if ( selection->isTopLevelGroup() )
    {
        // When dropping something to "Network" or its subdirs, we want to create
        // a desktop link, not to move/copy/link
        QString path = selection->topLevelItem()->path();
        KURL::List lst;
        if ( KURLDrag::decode( ev, lst ) ) // Are they urls ?
        {
            KURL::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
            {
                const KURL & targetURL = (*it);
                KURL linkURL;
                linkURL.setPath( path );
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
            kdError(1202) << "Dropped to a remote item !  " << url.prettyURL() << endl;
    }
    else
    {
        if ( selection->isTopLevelItem() )
            KonqOperations::doDrop( 0L, selection->externalURL(), ev, this );
        else
            selection->drop( ev, this );
    }
}

void KonqTree::contentsMousePressEvent( QMouseEvent *e )
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

void KonqTree::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( !m_bDrag || ( e->pos() - m_dragPos ).manhattanLength() <= KGlobalSettings::dndEventDelay() )
        return;

    m_bDrag = false;

    QListViewItem *item = itemAt( contentsToViewport( m_dragPos ) );

    if ( !item || !item->isSelectable() )
        return;

    // Start a drag
    QDragObject * drag = static_cast<KonqTreeItem *>( item )->dragObject( viewport(), false );
    drag->drag();
}

void KonqTree::contentsMouseReleaseEvent( QMouseEvent *e )
{
    KListView::contentsMouseReleaseEvent( e );
    m_bDrag = false;
}

void KonqTree::slotDoubleClicked( QListViewItem *item )
{
    if ( !item )
        return;

    if ( !static_cast<KonqTreeItem*>(item)->isClickable() )
        return;

    slotClicked( item );
    item->setOpen( !item->isOpen() );
}

void KonqTree::slotClicked( QListViewItem *item )
{
    if ( !item )
        return;

    if ( !static_cast<KonqTreeItem*>(item)->isClickable() )
        return;

    KonqTreeItem *dItem = static_cast<KonqTreeItem *>( item );

    KParts::URLArgs args;

    args.serviceType = dItem->externalMimeType();
    args.trustedSource = true;
    emit m_part->extension()->openURLRequest( dItem->externalURL(), args );
}

void KonqTree::slotRightButtonPressed( QListViewItem *_item )
{
    if ( !_item )
        return;

    item->setSelected( true );
    KonqTreeItem * item = static_cast<KonqTreeItem *>(_item);
    if ( item->isTopLevelGroup() )
    {
        KURL url;
        url.setPath( item->topLevelItem()->path() );
        emit m_part->extension()->popupMenu( QCursor::pos(), url, "inode/directory" );
    }
    else if ( item->isTopLevelItem() )
    {
        KURL url;
        url.setPath( item->topLevelItem()->path() );
        emit m_part->extension()->popupMenu( QCursor::pos(), url, "application/x-desktop" );
    }
    else
        item->rightButtonPressed();
}

void KonqTree::slotMouseButtonPressed(int _button, QListViewItem* _item, const QPoint&, int col)
{
    KonqTreeItem * item = static_cast<KonqTreeItem*>(_item);
    if(_item && _button == MidButton && col < 2)
        item->module()->mmbClicked( item );
}

void KonqTree::slotAnimation()
{
    QPixmap gearPixmap = SmallIcon( QString::fromLatin1( "kde" ).append( QString::number( m_animationCounter ) ), KonqTreeFactory::instance() );

    ListCurrentOpeningFolders::ConstIterator it = m_listCurrentOpeningFolders.begin();
    ListCurrentOpeningFolders::ConstIterator end = m_listCurrentOpeningFolders.end();
    for (; it != end; ++it )
        (*it)->setPixmap( 0, gearPixmap );

    m_animationCounter++;
    if ( m_animationCounter == 7 )
        m_animationCounter = 1;
}

void KonqTree::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

    if ( !m_dropItem || m_dropItem->isOpen() )
        return;

    m_dropItem->setOpen( true );
    m_dropItem->repaint();
}

void KonqTree::rescanConfiguration()
{
    kdDebug() << "KonqTree::rescanConfiguration()" << endl;
    m_autoOpenTimer->stop();
    clearTree();
    scanDir( 0, m_dirtreeDir.path(), true);
}

void KonqTree::FilesAdded( const KURL & dir )
{
    kdDebug(1202) << "KonqTree::FilesAdded " << dir.url() << endl;
    if ( m_dirtreeDir.isParentOf( dir ) )
        // We use a timer in case of DCOP re-entrance..
        QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
}

void KonqTree::FilesRemoved( const KURL::List & urls )
{
    //kdDebug(1202) << "KonqTree::FilesRemoved " << urls.count() << endl;
    for ( KURL::List::ConstIterator it = urls.begin() ; it != urls.end() ; ++it )
    {
        //kdDebug(1202) <<  "KonqTree::FilesRemoved " << (*it).prettyURL() << endl;
        if ( m_dirtreeDir.isParentOf( *it ) )
        {
            QTimer::singleShot( 0, this, SLOT( rescanConfiguration() ) );
            kdDebug(1202) << "KonqTree::FilesRemoved done" << endl;
            return;
        }
    }
}

void KonqTree::FilesChanged( const KURL::List & urls )
{
    //kdDebug(1202) << "KonqTree::FilesChanged" << endl;
    // not same signal, but same implementation
    FilesRemoved( urls );
}

void KonqTree::scanDir( KonqTreeItem *parent, const QString &path, bool isRoot )
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
        QString dirtree_dir = KonqTreeFactory::instance()->dirs()->findResourceDir( "data", "konqueror/dirtree/home.desktop" );

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

void KonqTree::scanDir2( KonqTreeItem *parent, const QString &path )
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

    KonqTreeItem *item;
    if ( parent )
        item = new KonqTreeItem( this, parent, 0 );
    else
        item = new KonqTreeItem( this, 0 );
    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( icon ) );
    item->setListable( false );
    item->setClickable( false );
    item->setTopLevelGroup( true );
    item->setOpen( open );

    m_topLevelItems.append( KonqTreeTopLevelItem( item, 0 /* no module */, path ) );

    kdDebug(1202) << "Inserting group " << name << "   " << path << endl;;

    scanDir( item, path );

    if ( item->childCount() == 0 )
        item->setExpandable( false );
}

void KonqTree::loadTopLevelItem( KonqTreeItem *parent,  const QString &filename )
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
    KonqTreeItem *item;
    if ( parent )
        item = new KonqTreeItem( this, parent, 0, fileItem );
    else
        item = new KonqTreeItem( this, 0, fileItem );

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

    KonqTreeModule * module = 0L;
    /////////// ####### !!!!!!!!! @@@@@@@@ here's where we need to create the right module...
#warning TODO
    m_topLevelItems.append( KonqTreeTopLevelItem( item, module, path ) );

    addSubDir( item, item, targetURL );

    bool open = cfg.readBoolEntry( "Open", false /* targetURL.isLocalFile() sucks a bit IMHO - DF */ );

    if ( !bListable )
        item->setListable( false );

    if ( open && item->isExpandable() )
        item->setOpen( true );

}

void KonqTree::stripIcon( QString &icon )
{
    QFileInfo info( icon );
    icon = info.baseName();
}

void KonqTree::startAnimation( KonqTreeItem * item, const char * iconBaseName )
{

}

void KonqTree::stopAnimation( KonqTreeItem * item )
{

}

#include "konq_tree.moc"
