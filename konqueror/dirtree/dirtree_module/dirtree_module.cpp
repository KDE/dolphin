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

#include <konq_treepart.h>
#include "dirtree_module.h"
#include "dirtree_item.h"
#include <kdebug.h>
#include <konq_operations.h>
#include <konq_propsview.h>
#include <konq_drag.h>
#include <kglobalsettings.h>
#include <kprotocolinfo.h>
#include <kdesktopfile.h>
#include <kio/paste.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <assert.h>

KonqDirTreeModule::KonqDirTreeModule( KonqTree * parentTree )
    : KonqTreeModule( parentTree ), m_dirLister( 0L )
{
    // Used to be static...
    s_defaultViewProps = new KonqPropsView( KonqTreeFactory::instance(), 0L );
    // Create a properties instance for this view
    m_pProps = new KonqPropsView( KonqTreeFactory::instance(), s_defaultViewProps );
}

void KonqDirTreeModule::clearAll()
{
    delete m_pProps;
    delete s_defaultViewProps;

    delete m_dirLister;
}

QDragObject * KonqDirTreeModule::dragObject( QWidget * parent, bool move )
{
    KonqDirTreeItem *item = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );

    if ( !item )
        return 0L;

    KURL::List lst;
    lst.append( item->fileItem()->url() );

    KonqDrag * drag = KonqDrag::newDrag( lst, false, parent );

    QPoint hotspot;
    hotspot.setX( item->pixmap( 0 )->width() / 2 );
    hotspot.setY( item->pixmap( 0 )->height() / 2 );
    drag->setPixmap( *(item->pixmap( 0 )), hotspot );
    drag->setMoveSelection( move );

    return drag;
}

void KonqDirTreeModule::paste()
{
    // move or not move ?
    bool move = false;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) ) {
        move = KonqDrag::decodeIsCutSelection( data );
        kdDebug(1202) << "move (from clipboard data) = " << move << endl;
    }

    KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );
    assert( selection );
    KIO::pasteClipboard( selection->fileItem()->url(), move );
}


KURL::List KonqDirTreeModule::selectedUrls()
{
    KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );
    assert( selection );
    KURL::List lst;
    lst.append(selection->fileItem()->url());
    return lst;
}

void KonqDirTreeModule::trash()
{
    KonqOperations::del(m_pTree,
                        KonqOperations::TRASH,
                        selectedUrls());
}

void KonqDirTreeModule::del()
{
    KonqOperations::del(m_pTree,
                        KonqOperations::DEL,
                        selectedUrls());
}

void KonqDirTreeModule::shred()
{
    KonqOperations::del(m_pTree,
                        KonqOperations::SHRED,
                        selectedUrls());
}


void KonqDirTreeModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    KDesktopFile cfg( item->path(), true );
    cfg.setDollarExpansion(true);
    QString icon;

    KURL targetURL;
    targetURL.setPath(item->path());

    if ( cfg.hasLinkType() )
    {
        targetURL = cfg.readURL();
        icon = cfg.readIcon();
        //stripIcon( icon );
    }
    else if ( cfg.hasDeviceType() )
    {
        // Determine the mountpoint
        QString mp = cfg.readEntry("MountPoint");
        if ( mp.isEmpty() )
            return;

        targetURL.setPath(mp);
        icon = cfg.readIcon();
    }
    else
        return;

    bool bListable = KProtocolInfo::supportsListing( targetURL.protocol() );
    kdDebug() << targetURL.prettyURL() << " listable : " << bListable << endl;

    if ( bListable )
    {
        m_dirLister = new KonqDirLister( true );
        m_dirLister->setDirOnlyMode( true );

        connect( m_dirLister, SIGNAL( newItems( const KFileItemList & ) ),
                 this, SLOT( slotNewItems( const KFileItemList & ) ) );
        connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                 this, SLOT( slotDeleteItem( KFileItem * ) ) );
        connect( m_dirLister, SIGNAL( completed() ),
                 this, SLOT( slotListingStopped() ) );
        connect( m_dirLister, SIGNAL( canceled() ),
                 this, SLOT( slotListingStopped() ) );
        connect( m_dirLister, SIGNAL( redirection( const KURL &) ),
                 this, SLOT( slotRedirection( const KURL &) ) );
    }
    else
    {
        item->setExpandable( false );
        item->setListable( false );
    }

    item->setPixmap( 0, SmallIcon( icon ) );
    item->setExternalURL( targetURL );
    addSubDir( item, targetURL );
}


void KonqDirTreeModule::openSubFolder( KonqDirTreeItem *item )
{
    KURL u = item->externalURL();

    kdDebug(1202) << "openSubFolder( " << u.url() << " )  topLevel=" << topLevel << endl;

    if ( m_dirLister->job() == 0 )
    {
        //kdDebug() << "KonqDirTreeModule::openSubFolder m_currentlyListedURL=" << m_currentlyListedURL.prettyURL() << endl;
        //kdDebug() << "KonqDirTreeModule::openSubFolder dirlister=" << m_dirLister << endl;
        openDirectory( u, topLevel ? true : false );
    }
    else  if ( !m_lstPendingURLs->contains( u ) )
        m_lstPendingURLs->append( u );

    if ( item->isTopLevelItem() ) // No animation for toplevel items (breaks the icon)
        return;

#if 0
    kdDebug(1202) << "m_mapCurrentOpeningFolders.insert( " << u.prettyURL() << " )" << endl;
    m_mapCurrentOpeningFolders.insert( u, item );
#endif

    //startAnimation( item, "kde" );
}

void KonqDirTreeModule::slotNewItems( const KFileItemList& entries )
{
    kdDebug(1202) << "KonqDirTreeModule::slotNewItems " << entries.count() << endl;

    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

    QListIterator<KFileItem> kit ( entries );
    for( ; kit.current(); ++kit )
    {
        KonqFileItem * item = static_cast<KonqFileItem *>(*kit);

        assert( S_ISDIR( item->mode() ) );

        KURL dir( item->url() );
        dir.setFileName( "" );
        //KURL dir( item->url().directory() );

        //  KonqDirTreeItem *parentDir = findDir( m_mapSubDirs, dir.url( 0 ) );
        //  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = m_mapSubDirs.find( dir );
        // *mumble* can't use QMap::find() because the cmp doesn't ignore the trailing slash :-(
        QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = m_mapSubDirs.begin();
        QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = m_mapSubDirs.end();
        for (; dirIt != dirEnd; ++dirIt )
        {
            if ( dir.cmp( dirIt.key(), true ) )
                break;
        }

        if( dirIt == m_mapSubDirs.end() )
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

        KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( this, parentDir, m_item, item );
        dirTreeItem->setPixmap( 0, m_folderPixmap );
        dirTreeItem->setText( 0, KIO::decodeFileName( item->url().fileName() ) );
    }
}

void KonqDirTreeModule::slotDeleteItem( KFileItem *item )
{
    assert( S_ISDIR( item->mode() ) );

    //  kdDebug(1202) << "slotDeleteItem( " << item->url().url() << " )" << endl;

    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

    if ( item == m_item->fileItem() )
    {
        assert( 0 ); //TODO ;)
    }

    QListViewItemIterator it( m_item );
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

void KonqDirTreeModule::slotRedirection( const KURL & url )
{
    kdDebug(1202) << "KonqDirTree::slotRedirection(" << url.prettyURL() << ")" << endl;
    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

    KURL dir = m_currentlyListedURL;
    QMap<KURL, KonqDirTreeItem *>::Iterator dirIt = m_mapSubDirs.begin();
    QMap<KURL, KonqDirTreeItem *>::Iterator dirEnd = m_mapSubDirs.end();
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
        m_mapSubDirs.remove( dirIt );
        m_mapSubDirs.insert( url, item );
        kdDebug() << "Updating url to " << url.prettyURL() << endl;
    }
}

void KonqDirTreeModule::slotListingStopped()
{
    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );
    KURL url = lister->url();

    kdDebug() << "KonqDirTree::slotListingStopped " << url.prettyURL() << endl;

    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = m_mapSubDirs.begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = m_mapSubDirs.end();
    for (; dirIt != dirEnd; ++dirIt )
    {
        if ( url.cmp( dirIt.key(), true ) )
            break;
    }

    if ( dirIt != m_mapSubDirs.end() && dirIt.data()->childCount() == 0 )
    {
        dirIt.data()->setExpandable( false );
        dirIt.data()->repaint();
    }

    KURL::List::Iterator it = m_lstPendingURLs.find( url );
    if ( it != m_lstPendingURLs.end() )
    {
        m_lstPendingURLs.remove( it );
    }

    if ( m_lstPendingURLs.count() > 0 )
    {
        kdDebug(1202) << "opening (was pending) " << m_lstPendingURLs.first().prettyURL() << endl;
        openDirectory( m_lstPendingURLs.first(), true );
    }

    kdDebug(1202) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL() == url )
    {
        kdDebug(1202) << "Selecting m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
        followURL( m_selectAfterOpening );
        m_selectAfterOpening = KURL();
    }
#if 0
    QMap<KURL, QListViewItem *>::Iterator oIt = m_mapCurrentOpeningFolders.find( url );
    if ( oIt != m_mapCurrentOpeningFolders.end() )
    {
        oIt.data()->setPixmap( 0, m_folderPixmap );

        m_mapCurrentOpeningFolders.remove( oIt );

        if ( m_mapCurrentOpeningFolders.count() == 0 )
            m_animationTimer->stop();
    }// else kdDebug(1202) << url.prettyURL() << "not found in m_mapCurrentOpeningFolders" << endl;
#endif
}

void KonqDirTreeModule::openDirectory( const KURL & url, bool keep )
{
    m_currentlyListedURL = url;

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    bool newProps = m_pProps->enterDir( url );

    m_dirLister->openURL( url, m_view->m_pProps->isShowingDotFiles(), keep );

    if ( newProps )
    {
        // See the other parts
        m_pProps->applyColors( viewport() );
    }
}

#include "dirtree_module.moc"
