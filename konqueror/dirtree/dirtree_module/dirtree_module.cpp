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
#include <konq_propsview.h>
#include <konq_drag.h>
#include <kglobalsettings.h>
#include <kprotocolinfo.h>
#include <kdesktopfile.h>
#include <assert.h>

KonqDirTreeModule::KonqDirTreeModule( KonqTree * parentTree )
    : KonqTreeModule( parentTree ), m_dirLister(0L), m_topLevelItem(0L), m_pProps(0L)
{
    // Used to be static...
    s_defaultViewProps = new KonqPropsView( KonqTreeFactory::instance(), 0L );
}

KonqDirTreeModule::~KonqDirTreeModule()
{
    delete m_pProps;
    delete s_defaultViewProps;
    delete m_dirLister;
}

KURL::List KonqDirTreeModule::selectedUrls()
{
    KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );
    assert( selection );
    KURL::List lst;
    lst.append(selection->fileItem()->url());
    return lst;
}

void KonqDirTreeModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    assert(!m_topLevelItem); // We can handle only one at a time !

    KDesktopFile cfg( item->path(), true );
    cfg.setDollarExpansion(true);

    KURL targetURL;
    targetURL.setPath(item->path());

    if ( cfg.hasLinkType() )
    {
        targetURL = cfg.readURL();
    }
    else if ( cfg.hasDeviceType() )
    {
        // Determine the mountpoint
        QString mp = cfg.readEntry("MountPoint");
        if ( mp.isEmpty() )
            return;

        targetURL.setPath(mp);
    }
    else
        return;

    bool bListable = KProtocolInfo::supportsListing( targetURL.protocol() );
    kdDebug(1201) << targetURL.prettyURL() << " listable : " << bListable << endl;

    if ( !bListable )
    {
        item->setExpandable( false );
        item->setListable( false );
    }

    item->setExternalURL( targetURL );
    addSubDir( item );

    m_topLevelItem = item;
}

void KonqDirTreeModule::openTopLevelItem( KonqTreeTopLevelItem * item )
{
    if ( !item->childCount() && item->isListable() )
        openSubFolder( item );
}

void KonqDirTreeModule::addSubDir( KonqTreeItem *item )
{
    kdDebug(1201) << this << " KonqDirTreeModule::addSubDir " << item->externalURL().url(-1) << endl;
    m_dictSubDirs.insert( item->externalURL().url(-1), item );
}

void KonqDirTreeModule::removeSubDir( KonqTreeItem *item )
{
    m_dictSubDirs.remove( item->externalURL().url(-1) );
}


void KonqDirTreeModule::openSubFolder( KonqTreeItem *item )
{
    kdDebug(1201) << this << " openSubFolder( " << item->externalURL().prettyURL() << " )" << endl;

    if ( !m_dirLister ) // created on demand
    {
        m_dirLister = new KonqDirLister( true );
        m_dirLister->setDirOnlyMode( true );

        connect( m_dirLister, SIGNAL( newItems( const KFileItemList & ) ),
                 this, SLOT( slotNewItems( const KFileItemList & ) ) );
        connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList & ) ),
                 this, SLOT( slotRefreshItems( const KFileItemList & ) ) );
        connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                 this, SLOT( slotDeleteItem( KFileItem * ) ) );
        connect( m_dirLister, SIGNAL( completed() ),
                 this, SLOT( slotListingStopped() ) );
        connect( m_dirLister, SIGNAL( canceled() ),
                 this, SLOT( slotListingStopped() ) );
        connect( m_dirLister, SIGNAL( redirection( const KURL &, const KURL & ) ),
                 this, SLOT( slotRedirection( const KURL &, const KURL & ) ) );
    }

    if ( !m_pProps ) // created on demand
    {
        // Create a properties instance for this view
        m_pProps = new KonqPropsView( KonqTreeFactory::instance(), s_defaultViewProps );
    }

    if ( m_dirLister->job() == 0 )
        listDirectory( item );
    else if ( ! m_lstPendingOpenings.contains( item ) )
        m_lstPendingOpenings.append( item );

    m_pTree->startAnimation( item );
}

void KonqDirTreeModule::listDirectory( KonqTreeItem *item )
{
    // This causes a reparsing, but gets rid of the trailing slash
    KURL url( item->externalURL().url(-1) );

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    /*bool newProps = */m_pProps->enterDir( url );

    m_dirLister->openURL( url, m_pProps->isShowingDotFiles(), true /*keep*/ );

#if 0
    if ( newProps )
    {
        // See the other parts
        m_pProps->applyColors( viewport() );
    }
#endif
}

void KonqDirTreeModule::slotNewItems( const KFileItemList& entries )
{
    kdDebug(1201) << this << " KonqDirTreeModule::slotNewItems " << entries.count() << endl;

    assert(entries.count());
    KFileItem * firstItem = const_cast<KFileItemList&>(entries).first(); // qlist sucks for constness

    // Find parent item - it's the same for all the items
    KURL dir( firstItem->url() );
    dir.setFileName( "" );
    kdDebug(1201) << this << " KonqDirTreeModule::slotNewItems dir=" << dir.url(-1) << endl;
    KonqTreeItem * parentItem = m_dictSubDirs[ dir.url(-1) ];
    assert( parentItem );

    int size = KGlobal::iconLoader()->currentSize( KIcon::Small );
    QListIterator<KFileItem> kit ( entries );
    for( ; kit.current(); ++kit )
    {
        KonqFileItem * fileItem = static_cast<KonqFileItem *>(*kit);
        assert( fileItem->isDir() );

        KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( parentItem, m_topLevelItem, fileItem );
        dirTreeItem->setPixmap( 0, fileItem->pixmap( size ) );
        dirTreeItem->setText( 0, KIO::decodeFileName( fileItem->url().fileName() ) );
    }
}

void KonqDirTreeModule::slotRefreshItems( const KFileItemList &entries )
{
    int size = KGlobal::iconLoader()->currentSize( KIcon::Small );
    QListIterator<KFileItem> kit ( entries );
    for( ; kit.current(); ++kit )
    {
        // All items are in m_dictSubDirs, so look them up fast
        KonqTreeItem * item = m_dictSubDirs[ kit.current()->url().url(-1) ];
        if ( !item->isTopLevelItem() ) // we only have dirs and one toplevel item in the dict
        {
            KonqDirTreeItem * dirTreeItem = static_cast<KonqDirTreeItem *>(item);
            KonqFileItem * fileItem = dirTreeItem->fileItem();
            dirTreeItem->setPixmap( 0, fileItem->pixmap( size ) );
            dirTreeItem->setText( 0, KIO::decodeFileName( fileItem->url().fileName() ) );
        }
    }
}

void KonqDirTreeModule::slotDeleteItem( KFileItem *fileItem )
{
    assert( fileItem->isDir() );

    kdDebug(1201) << "KonqDirTreeModule::slotDeleteItem( " << fileItem->url().url(-1) << " )" << endl;

    // All items are in m_dictSubDirs, so look it up fast
    KonqTreeItem * item = m_dictSubDirs[ fileItem->url().url(-1) ];
    ASSERT(item);
    if (item)
    {
        removeSubDir( item );
        delete item;
    }
}

void KonqDirTreeModule::slotRedirection( const KURL & oldUrl, const KURL & newUrl )
{
    kdDebug(1201) << "KonqDirTreeModule::slotRedirection(" << newUrl.prettyURL() << ")" << endl;

    KonqTreeItem * item = m_dictSubDirs[ oldUrl.url(-1) ];
    ASSERT( item );

    if (!item)
        kdWarning(1201) << "NOT FOUND   oldUrl=" << oldUrl.prettyURL() << endl;
    else
    {
        // We need to update the URL in m_dictSubDirs
        m_dictSubDirs.remove( oldUrl.url(-1) );
        m_dictSubDirs.insert( newUrl.url(-1), item );
        kdDebug(1201) << "Updating url to " << newUrl.prettyURL() << endl;
    }
}

void KonqDirTreeModule::slotListingStopped()
{
    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );
    KURL url = lister->url();
    KonqTreeItem * item = m_dictSubDirs[ url.url(-1) ];

    ASSERT(item);

    kdDebug(1201) << "KonqDirTree::slotListingStopped " << url.prettyURL() << endl;

    if ( item && item->childCount() == 0 )
    {
        item->setExpandable( false );
        item->repaint();
    }

    m_lstPendingOpenings.removeRef( item );

    if ( m_lstPendingOpenings.count() > 0 )
        listDirectory( m_lstPendingOpenings.first() );

    kdDebug(1201) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL().cmp(url,true) )
    {
        kdDebug(1201) << "Selecting m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
        followURL( m_selectAfterOpening );
        m_selectAfterOpening = KURL();
    }

    m_pTree->stopAnimation( item );
}

void KonqDirTreeModule::followURL( const KURL & url )
{
    // Check if we already know this URL
    KonqTreeItem * item = m_dictSubDirs[ url.url(-1) ];
    if (item) // found it  -> ensure visible, select, return.
    {
        m_pTree->ensureItemVisible( item );
        m_pTree->setSelected( item, true );
        return;
    }

    KURL uParent( url.upURL() );
    KonqTreeItem * parentItem = m_dictSubDirs[ uParent.url(-1) ];
    if (!parentItem)
        return;

    // That's the parent directory. Open if not open...
    if ( !parentItem->isOpen() )
    {
        parentItem->setOpen( true );
        if ( parentItem->childCount() )
        {
            // Immediate opening, if the dir was already listed
            followURL( url );
            return;
        } else
        {
            m_selectAfterOpening = url;
            kdDebug(1202) << "KonqDirTreeModule::followURL: m_selectAfterOpening=" << m_selectAfterOpening.url() << endl;
            return; // We know we won't find it
        }
    }
}

#include "dirtree_module.moc"
