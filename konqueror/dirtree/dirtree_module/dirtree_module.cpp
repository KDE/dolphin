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
    : KonqTreeModule( parentTree ), /*m_lasttvd(0L), */m_dirLister(0L), m_topLevelItem(0L), m_pProps(0L)
{
    // Used to be static...
    s_defaultViewProps = new KonqPropsView( KonqTreeFactory::instance(), 0L );

    m_folderPixmap = KMimeType::mimeType( "inode/directory" )->pixmap( KIcon::Desktop, KIcon::SizeSmall );

}

void KonqDirTreeModule::clearAll()
{
    delete m_pProps;
    delete s_defaultViewProps;
    //### TODO iterate over the map and kill the listers
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
    assert(!m_topLevelItem); // We can handle only one at a time !

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

    if ( !bListable )
    {
        item->setExpandable( false );
        item->setListable( false );
    }

    item->setPixmap( 0, SmallIcon( icon ) );
    item->setExternalURL( targetURL );
    addSubDir( item );

    m_topLevelItem = item;
}


void KonqDirTreeModule::addSubDir( KonqTreeItem *item )
{
    m_dictSubDirs.insert( item->externalURL().url(0), item );
}

void KonqDirTreeModule::removeSubDir( KonqTreeItem *item )
{
    //m_lasttvd = 0L; // drop cache, to avoid segfaults
    m_dictSubDirs.remove( item->externalURL().url(0) );
}


void KonqDirTreeModule::openSubFolder( KonqTreeItem *item )
{
    // This causes a reparsing, but gets rid of the trailing slash
    KURL url( item->externalURL().url(0) );

    kdDebug(1202) << "openSubFolder( " << url.url() << " )" << endl;

    if ( !m_dirLister ) // created on demand
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
        connect( m_dirLister, SIGNAL( redirection( const KURL & oldUrl, const KURL & newUrl ) ),
                 this, SLOT( slotRedirection( const KURL & oldUrl, const KURL & newUrl ) ) );
    }

    if ( !m_pProps ) // created on demand
    {
        // Create a properties instance for this view
        m_pProps = new KonqPropsView( KonqTreeFactory::instance(), s_defaultViewProps );
    }

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    /*bool newProps = */m_pProps->enterDir( url );

    m_dirLister->openURL( url, m_pProps->isShowingDotFiles(), item->isTopLevelItem() /*keep*/ );

#if 0
    if ( newProps )
    {
        // See the other parts
        m_pProps->applyColors( viewport() );
    }
#endif

    if ( item->isTopLevelItem() ) // No animation for toplevel items (breaks the icon)
        return;

    //startAnimation( item, "kde" );
}

void KonqDirTreeModule::slotNewItems( const KFileItemList& entries )
{
    kdDebug(1202) << "KonqDirTreeModule::slotNewItems " << entries.count() << endl;

    QListIterator<KFileItem> kit ( entries );
    for( ; kit.current(); ++kit )
    {
        KonqFileItem * fileItem = static_cast<KonqFileItem *>(*kit);

        assert( fileItem->isDir() );

        // Find parent item
        KURL dir( fileItem->url() );
        dir.setFileName( "" );
        KonqTreeItem * parentItem = m_dictSubDirs[ dir.url(0) ];
        ASSERT( parentItem );

        KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( m_pTree, parentItem, m_topLevelItem, fileItem );
        dirTreeItem->setPixmap( 0, m_folderPixmap );
        dirTreeItem->setText( 0, KIO::decodeFileName( fileItem->url().fileName() ) );
    }
}

void KonqDirTreeModule::slotDeleteItem( KFileItem *fileItem )
{
    assert( fileItem->isDir() );

    kdDebug() << "KonqDirTreeModule::slotDeleteItem( " << fileItem->url().url(0) << " )" << endl;

    /* No, thanks the slow method isn't what we want ;)
    QListViewItemIterator it( m_item );
    for (; it.current(); ++it )
    {
        if ( static_cast<KonqDirTreeItem *>( it.current() )->fileItem() == fileItem )
        {
            //      kdDebug(1202) << "removing " << item->url().url() << endl;
            delete it.current();
            return;
        }
    }*/

    // All items are in m_dictSubDirs, so look it up fast
    KonqTreeItem * item = m_dictSubDirs[ fileItem->url().url(0) ];
    ASSERT(item);
    if (item)
        delete item;
}

void KonqDirTreeModule::slotRedirection( const KURL & oldUrl, const KURL & newUrl )
{
    kdDebug(1202) << "KonqDirTreeModule::slotRedirection(" << newUrl.prettyURL() << ")" << endl;

    KonqTreeItem * item = m_dictSubDirs[ oldUrl.url(0) ];
    ASSERT( item );

    if (!item)
        kdWarning() << "NOT FOUND   oldUrl=" << oldUrl.prettyURL() << endl;
    else
    {
        // We need to update the URL in m_dictSubDirs
        m_dictSubDirs.remove( oldUrl.url(0) );
        m_dictSubDirs.insert( newUrl.url(0), item );
        kdDebug() << "Updating url to " << newUrl.prettyURL() << endl;
    }
}

void KonqDirTreeModule::slotListingStopped()
{
    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );
    KURL url = lister->url();
    KonqTreeItem * item = m_dictSubDirs[ url.url(0) ];

    ASSERT(item);

    kdDebug() << "KonqDirTree::slotListingStopped " << url.prettyURL() << endl;

    if ( item && item->childCount() == 0 )
    {
        item->setExpandable( false );
        item->repaint();
    }

#if 0
    KURL::List::Iterator it = m_lstPendingURLs.find( url );
    if ( it != m_lstPendingURLs.end() )
        m_lstPendingURLs.remove( it );

    if ( m_lstPendingURLs.count() > 0 )
    {
        kdDebug(1202) << "opening (was pending) " << m_lstPendingURLs.first().prettyURL() << endl;
        openDirectory( m_lstPendingURLs.first(), true );
    }
#endif

    kdDebug(1202) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL() == url )
    {
        kdDebug(1202) << "Selecting m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
        /// ### TODO followURL( m_selectAfterOpening );
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

/*
KonqTreeItem * KonqDirTreeModule::findDir( const KURL &_url )
{
    // Heavily copied from KonqTreeViewWidget
    QString url = _url.url(0);
    if ( m_lasttvd && urlcmp( m_lasttvd->url(0), url, true, true ) )
        return m_lasttvd;

    QDictIterator<KonqTreeItem> it( m_dictSubDirs );
    for( ; it.current(); ++it )
    {
        kdDebug(1202) << it.current()->externalURL().url(0) << endl;
        if ( urlcmp( it.current()->externalURL().url(0), url, true, true ) )
        {
            m_lasttvd = it.current();
            return it.current();
        }
    }
    return 0L;
}*/

#include "dirtree_module.moc"
