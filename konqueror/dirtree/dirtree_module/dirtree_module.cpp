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

#include "dirtree_module.h"
#include "dirtree_item.h"
#include <konq_operations.h>

KonqPropsView * KonqDirTreeModule::s_defaultViewProps = 0L;

static KonqPropsView *KonqDirTreeModule::defaultViewProps()
{
    if ( !s_defaultViewProps )
        s_defaultViewProps = new KonqPropsView( instance(), 0L );

    return s_defaultViewProps;
}

/*  TODO !
    if ( s_defaultViewProps )
    {
        delete s_defaultViewProps;
        s_defaultViewProps = 0L;
    }
*/

KonqDirTreeModule::DirTreeModule( KonqTree * parentTree, const char * name )
    : KonqTreeModule( parentTree, name )
{
    // Create a properties instance for this view
    m_pProps = new KonqPropsView( KonqDirTreeFactory::instance(), KonqDirTreeFactory::defaultViewProps() );

}

void KonqDirTreeModule::clearAll()
{
    delete m_dirLister;
    delete m_mapSubDirs;
    delete m_lstPendingURLs;
}

QDragObject * KonqDirTreeModule::dragObject( QWidget * parent, bool move )
{
    KonqDirTreeItem *item = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );

    if ( !item )
        return 0L;

    KURL::List lst;
    lst.append( item->fileItem()->url() );

    QDragObject * drag = KonqDrag::newDrag( lst, false, parent );

    QPoint hotspot;
    hotspot.setX( item->pixmap( 0 )->width() / 2 );
    hotspot.setY( item->pixmap( 0 )->height() / 2 );
    drag->setPixmap( *(item->pixmap( 0 )), hotspot );

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

void KonqDirTreeModule::slotSelectionChanged()
{
    bool cutcopy, del;
    bool bInTrash = false;

    KonqDirTreeItem *selection = static_cast<KonqDirTreeItem *>( m_pTree->selectedItem() );

    if ( selection && selection->fileItem()->url().directory(false) == KGlobalSettings::trashPath() )
        bInTrash = true;

    cutcopy = del = selection;

    emit enableAction( "copy", cutcopy );
    emit enableAction( "cut", cutcopy );
    emit enableAction( "trash", del && !bInTrash );
    emit enableAction( "del", del );
    emit enableAction( "shred", del );

    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = ( data->encodedData( data->format() ).size() != 0 ) && selection;

    emit enableAction( "paste", paste );
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
    KonqOperations::del(m_tree,
                        KonqOperations::TRASH,
                        selectedUrls());
}

void KonqDirTreeModule::del()
{
    KonqOperations::del(m_tree,
                        KonqOperations::DEL,
                        selectedUrls());
}

void KonqDirTreeModule::shred()
{
    KonqOperations::del(m_tree,
                        KonqOperations::SHRED,
                        selectedUrls());
}


void KonqDirTreeModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    KDesktopFile cfg( item->path(), true );
    cfg.setDollarExpansion(true);

    KURL targetURL;
    targetURL.setPath(path);

    if ( cfg.hasLinkType() )
    {
        targetURL = cfg.readURL();
        icon = cfg.readIcon();
        stripIcon( icon );
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
    {
        item->setExpandable( false );
        item->setListable( false );
    }

    item->setExternalURL( targetURL );
    addSubDir( item, item, targetURL );
}

void KonqDirTreeModule::slotNewItems( const KFileItemList& entries )
{
  kdDebug(1202) << "KonqDirTreeModule::slotNewItems " << entries.count() << endl;

  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem & topLevelItem = findTopLevelByDirLister( lister );
  kdDebug() << "KonqDirTreeModule::slotNewItems topLevelItem=" << &topLevelItem << endl;

  assert( topLevelItem.m_item );

  QListIterator<KFileItem> kit ( entries );
  for( ; kit.current(); ++kit )
  {
    KonqFileItem * item = static_cast<KonqFileItem *>(*kit);

    assert( S_ISDIR( item->mode() ) );

    KURL dir( item->url() );
    dir.setFileName( "" );
    //KURL dir( item->url().directory() );

    //  KonqDirTreeItem *parentDir = findDir( *topLevelItem.m_mapSubDirs, dir.url( 0 ) );
    //  QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->find( dir );
    // *mumble* can't use QMap::find() because the cmp doesn't ignore the trailing slash :-(
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = topLevelItem.m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = topLevelItem.m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
    {
      if ( dir.cmp( dirIt.key(), true ) )
        break;
    }

    if( dirIt == topLevelItem.m_mapSubDirs->end() )
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

    KonqDirTreeItem *dirTreeItem = new KonqDirTreeItem( this, parentDir, topLevelItem.m_item, item );
    dirTreeItem->setPixmap( 0, m_folderPixmap );
    dirTreeItem->setText( 0, KIO::decodeFileName( item->url().fileName() ) );
  }
}

void KonqDirTreeModule::slotDeleteItem( KFileItem *item )
{
  assert( S_ISDIR( item->mode() ) );

  //  kdDebug(1202) << "slotDeleteItem( " << item->url().url() << " )" << endl;

  const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );

  TopLevelItem topLevelItem = findTopLevelByDirLister( lister );

  assert( topLevelItem.m_item );

  if ( item == topLevelItem.m_item->fileItem() )
  {
    assert( 0 ); //TODO ;)
  }

  QListViewItemIterator it( topLevelItem.m_item );
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

    TopLevelItem & topLevelItem = findTopLevelByDirLister( lister );

    KURL dir = topLevelItem.m_currentlyListedURL;
    QMap<KURL, KonqDirTreeItem *>::Iterator dirIt = topLevelItem.m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::Iterator dirEnd = topLevelItem.m_mapSubDirs->end();
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
        topLevelItem.m_mapSubDirs->remove( dirIt );
        topLevelItem.m_mapSubDirs->insert( url, item );
        kdDebug() << "Updating url to " << url.prettyURL() << endl;
    }
}

void KonqDirTreeModule::slotListingStopped()
{
    const KonqDirLister *lister = static_cast<const KonqDirLister *>( sender() );
    KURL url = lister->url();

    kdDebug() << "KonqDirTree::slotListingStopped " << url.prettyURL() << endl;

    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirIt = m_mapSubDirs->begin();
    QMap<KURL, KonqDirTreeItem *>::ConstIterator dirEnd = m_mapSubDirs->end();
    for (; dirIt != dirEnd; ++dirIt )
    {
        if ( url.cmp( dirIt.key(), true ) )
            break;
    }

    if ( dirIt != m_mapSubDirs->end() && dirIt.data()->childCount() == 0 )
    {
        dirIt.data()->setExpandable( false );
        dirIt.data()->repaint();
    }

    KURL::List::Iterator it = m_lstPendingURLs->find( url );
    if ( it != m_lstPendingURLs->end() )
    {
        m_lstPendingURLs->remove( it );
    }

    if ( m_lstPendingURLs->count() > 0 )
    {
        kdDebug(1202) << "opening (was pending) " << m_lstPendingURLs->first().prettyURL() << endl;
        openDirectory( m_lstPendingURLs->first(), true );
    }

    kdDebug(1202) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    if ( !m_selectAfterOpening.isEmpty() && m_selectAfterOpening.upURL() == url )
    {
        kdDebug(1202) << "Selecting m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
        followURL( m_selectAfterOpening );
        m_selectAfterOpening = KURL();
    }

    QMap<KURL, QListViewItem *>::Iterator oIt = m_mapCurrentOpeningFolders.find( url );
    if ( oIt != m_mapCurrentOpeningFolders.end() )
    {
        oIt.data()->setPixmap( 0, m_folderPixmap );

        m_mapCurrentOpeningFolders.remove( oIt );

        if ( m_mapCurrentOpeningFolders.count() == 0 )
            m_animationTimer->stop();
    }// else kdDebug(1202) << url.prettyURL() << "not found in m_mapCurrentOpeningFolders" << endl;
}

void KonqDirTreeModule::openDirectory( const KURL & url, bool keep )
{
    m_currentlyListedURL = url;

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    bool newProps = m_view->m_pProps->enterDir( url );

    topLevelItem.m_dirLister->openURL( url, m_view->m_pProps->isShowingDotFiles(), keep );

    if ( newProps )
    {
        // See the other parts
        m_view->m_pProps->applyColors( viewport() );
    }
}

#include "dirtree_module.moc"
