/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2002 Michael Brade <brade@kde.org>

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
#include <kdebug.h>
#include <konq_propsview.h>
#include <kprotocolinfo.h>
#include <kdesktopfile.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kdirlister.h>


KonqSidebarDirTreeModule::KonqSidebarDirTreeModule( KonqSidebarTree * parentTree )
    : KonqSidebarTreeModule( parentTree ), m_dirLister(0L), m_topLevelItem(0L), m_pProps(0L)
{
    // Used to be static...
    s_defaultViewProps = new KonqPropsView(
	tree()->part()->parentInstance(),0);

    //KonqSidebarTreeFactory::instance(), 0L );
}

KonqSidebarDirTreeModule::~KonqSidebarDirTreeModule()
{
    delete m_pProps;
    delete s_defaultViewProps;
    delete m_dirLister;
}

KURL::List KonqSidebarDirTreeModule::selectedUrls()
{
    KURL::List lst;
    KonqSidebarDirTreeItem *selection = static_cast<KonqSidebarDirTreeItem *>( m_pTree->selectedItem() );
    if( !selection )
    {
        kdError() << "KonqSidebarDirTreeModule::selectedUrls: no selection!" << endl;
        return lst;
    }
    lst.append(selection->fileItem()->url());
    return lst;
}

void KonqSidebarDirTreeModule::addTopLevelItem( KonqSidebarTreeTopLevelItem * item )
{
    if(m_topLevelItem) // We can handle only one at a time !
        kdError() << "KonqSidebarDirTreeModule::addTopLevelItem: Impossible, we can have only one toplevel item !" << endl;

    KDesktopFile cfg( item->path(), true );
    cfg.setDollarExpansion(true);

    KURL targetURL;
    targetURL.setPath(item->path());

    if ( cfg.hasLinkType() )
    {
        targetURL = cfg.readURL();
		// some sevices might whant to make their URL confgurable in kcontol
		QString configured = cfg.readEntry("X-KDE-ConfiguredURL","");
		if (!configured.isEmpty()) {
			QStringList list = QStringList::split(':', configured);
			KConfig config(list[0]);
			if (list[1] != "noGroup") config.setGroup(list[1]);
			QString conf_url = config.readEntry(list[2]);
			if (!conf_url.isEmpty()) {
				targetURL = conf_url;
			}
		}
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
    //kdDebug(1201) << targetURL.prettyURL() << " listable : " << bListable << endl;

    if ( !bListable )
    {
        item->setExpandable( false );
        item->setListable( false );
    }

    item->setExternalURL( targetURL );
    addSubDir( item );

    m_topLevelItem = item;
}

void KonqSidebarDirTreeModule::openTopLevelItem( KonqSidebarTreeTopLevelItem * item )
{
    if ( !item->childCount() && item->isListable() )
        openSubFolder( item );
}

void KonqSidebarDirTreeModule::addSubDir( KonqSidebarTreeItem *item )
{
    KURL url( item->externalURL() );
    url.cleanPath();
    kdDebug(1201) << this << " KonqSidebarDirTreeModule::addSubDir " << url.url(-1) << endl;
    m_dictSubDirs.insert( url.url(-1), item );
}

void KonqSidebarDirTreeModule::removeSubDir( KonqSidebarTreeItem *item, bool childrenOnly )
{
    kdDebug(1201) << this << " KonqSidebarDirTreeModule::removeSubDir item=" << item << endl;
    if ( item->firstChild() )
    {
        KonqSidebarTreeItem * it = static_cast<KonqSidebarTreeItem *>(item->firstChild());
        KonqSidebarTreeItem * next = 0L;
        while ( it ) {
            next = static_cast<KonqSidebarTreeItem *>(it->nextSibling());
            removeSubDir( it );
            it = next;
        }
    }

    if ( !childrenOnly )
    {
        bool b = m_dictSubDirs.remove( item->externalURL().url(-1) );
	while (!(((KonqSidebarDirTreeItem*) item)->alias.isEmpty()))
	{
	        b = b|m_dictSubDirs.remove( ((KonqSidebarDirTreeItem*) item)->alias.front() );
		
		((KonqSidebarDirTreeItem*) item)->alias.pop_front();	
	}
        if (!b)
            kdWarning(1201) << this << " KonqSidebarDirTreeModule::removeSubDir item " << item
                            << " not found. URL=" << item->externalURL().url(-1) << endl;
    }
}


void KonqSidebarDirTreeModule::openSubFolder( KonqSidebarTreeItem *item )
{
    kdDebug(1201) << this << " openSubFolder( " << item->externalURL().prettyURL() << " )" << endl;

    if ( !m_dirLister ) // created on demand
    {
        m_dirLister = new KDirLister( true );
        m_dirLister->setDirOnlyMode( true );

        connect( m_dirLister, SIGNAL( newItems( const KFileItemList & ) ),
                 this, SLOT( slotNewItems( const KFileItemList & ) ) );
        connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList & ) ),
                 this, SLOT( slotRefreshItems( const KFileItemList & ) ) );
        connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                 this, SLOT( slotDeleteItem( KFileItem * ) ) );
        connect( m_dirLister, SIGNAL( completed( const KURL & ) ),
                 this, SLOT( slotListingStopped( const KURL & ) ) );
        connect( m_dirLister, SIGNAL( canceled( const KURL & ) ),
                 this, SLOT( slotListingStopped( const KURL & ) ) );
        connect( m_dirLister, SIGNAL( redirection( const KURL &, const KURL & ) ),
                 this, SLOT( slotRedirection( const KURL &, const KURL & ) ) );
    }

    if ( !m_pProps ) // created on demand
    {
        // Create a properties instance for this view
        //m_pProps = new KonqPropsView( KonqSidebarTreeFactory::instance(), s_defaultViewProps );
        m_pProps = new KonqPropsView(tree()->part()->parentInstance(), s_defaultViewProps );
    }

    if ( !item->isTopLevelItem() &&
         static_cast<KonqSidebarDirTreeItem *>(item)->hasStandardIcon() )
    {
        int size = KGlobal::iconLoader()->currentSize( KIcon::Small );
        QPixmap pix = DesktopIcon( "folder_open", size );
        m_pTree->startAnimation( item, "kde", 6, &pix );
    }
    else
        m_pTree->startAnimation( item );

    listDirectory( item );
}

void KonqSidebarDirTreeModule::listDirectory( KonqSidebarTreeItem *item )
{
    // This causes a reparsing, but gets rid of the trailing slash
    KURL url( item->externalURL().url(-1) );

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    /*bool newProps = */m_pProps->enterDir( url );
    m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
    
    if (tree()->isOpeningFirstChild()) m_dirLister->setAutoErrorHandlingEnabled(false,0);
	else m_dirLister->setAutoErrorHandlingEnabled(true,tree());
    m_dirLister->openURL( url, true /*keep*/ );

#if 0
    if ( newProps )
    {
        // See the other parts
        m_pProps->applyColors( viewport() );
    }
#endif
}

void KonqSidebarDirTreeModule::slotNewItems( const KFileItemList& entries )
{
    //kdDebug(1201) << this << " KonqSidebarDirTreeModule::slotNewItems " << entries.count() << endl;

    Q_ASSERT(entries.count());
    KFileItem * firstItem = const_cast<KFileItemList&>(entries).first(); // qlist sucks for constness

    // Find parent item - it's the same for all the items
    KURL dir( firstItem->url() );
    dir.setFileName( "" );
    dir.setPass( QString::null );
    kdDebug(1201) << this << " KonqSidebarDirTreeModule::slotNewItems dir=" << dir.url(-1) << endl;
    KonqSidebarTreeItem * parentItem = m_dictSubDirs[ dir.url(-1) ];
    if( !parentItem )
    {
        KMessageBox::error( tree(), i18n("Can't find parent item %1 in the tree. Internal error.").arg( dir.url(-1) ) );
       	return;
    }

    int size = KGlobal::iconLoader()->currentSize( KIcon::Small );
    QPtrListIterator<KFileItem> kit ( entries );
    for( ; kit.current(); ++kit )
    {
        KFileItem * fileItem = *kit;
        if (! fileItem->isDir() )
        {
            kdError() << "Item " << fileItem->url().prettyURL() << " is not a directory!" << endl;
            return;
        }

        KonqSidebarDirTreeItem *dirTreeItem = new KonqSidebarDirTreeItem( parentItem, m_topLevelItem, fileItem );
        dirTreeItem->setPixmap( 0, fileItem->pixmap( size ) );
        dirTreeItem->setText( 0, KIO::decodeFileName( fileItem->name() ) );
    }
}

void KonqSidebarDirTreeModule::slotRefreshItems( const KFileItemList &entries )
{
    int size = KGlobal::iconLoader()->currentSize( KIcon::Small );
    QPtrListIterator<KFileItem> kit ( entries );
    //kdDebug(1201) << "KonqSidebarDirTreeModule::slotRefreshItems " << entries.count() << " entries. First: " << kit.current()->url().url() << endl;
    // We can't look in m_dictSubDirs since the URL might have been updated (after a renaming)
    // so we have to use the long and painful method.
    for( ; kit.current(); ++kit )
    {
        QDictIterator<KonqSidebarTreeItem> it( m_dictSubDirs );
        for ( ; it.current(); ++it )
        {
            KonqSidebarTreeItem * item = it.current();
            if ( !item->isTopLevelItem() ) // we only have dirs and one toplevel item in the dict
            {
                KonqSidebarDirTreeItem * dirTreeItem = static_cast<KonqSidebarDirTreeItem *>(item);
                if ( dirTreeItem->fileItem() == kit.current() )
                {
                    //kdDebug(1201) << "KonqSidebarDirTreeModule::slotRefreshItems ** found match with new item " << kit.current()->url().url(-1) << endl;
                    //kdDebug(1201) << "KonqSidebarDirTreeModule::slotRefreshItems ** it.currentKey()=" << it.currentKey() << endl;
                    // Item renamed ?
                    if ( it.currentKey() != kit.current()->url().url( -1 ) )
                    {
                        // We need to update the URL in m_dictSubDirs, and to get rid of the child items, so remove and add.
                        // First remember toplevel and parent items
                        KonqSidebarTreeTopLevelItem * topLevelItem = dirTreeItem->topLevelItem();
                        KonqSidebarTreeItem * parentItem = static_cast<KonqSidebarTreeItem *>(dirTreeItem->parent());
                        // Then remove + delete
                        removeSubDir( dirTreeItem, true /*children only*/ );
                        // Only in it.currentKey() we have the old URL. The fileitem has the new one.
                        bool b = m_dictSubDirs.remove( it.currentKey() );
                        if ( !b )
                            kdWarning(1201) << "Couldn't remove URL " << it.currentKey() << " from dict" << endl;
                        delete dirTreeItem;
                        // And finally create a new item instead
                        dirTreeItem = new KonqSidebarDirTreeItem( parentItem, topLevelItem, kit.current() );
                    }
                    dirTreeItem->setPixmap( 0, kit.current()->pixmap( size ) );
                    dirTreeItem->setText( 0, KIO::decodeFileName( kit.current()->url().fileName() ) );
                    // addSubDir automatically called by constructor
                    break;
                }
            }
        }
    }
}

void KonqSidebarDirTreeModule::slotDeleteItem( KFileItem *fileItem )
{
    Q_ASSERT( fileItem->isDir() );

    kdDebug(1201) << "KonqSidebarDirTreeModule::slotDeleteItem( " << fileItem->url().url(-1) << " )" << endl;

    // All items are in m_dictSubDirs, so look it up fast
    KonqSidebarTreeItem * item = m_dictSubDirs[ fileItem->url().url(-1) ];
    Q_ASSERT(item);
    if (item)
    {
        removeSubDir( item );
        delete item;
    }
}

void KonqSidebarDirTreeModule::slotRedirection( const KURL & oldUrl, const KURL & newUrl )
{
    kdDebug(1201) << "******************************KonqSidebarDirTreeModule::slotRedirection(" << newUrl.prettyURL() << ")" << endl;

//    KMessageBox::error( tree(), "REDIRECTION" );

    KonqSidebarTreeItem * item = m_dictSubDirs[ oldUrl.url(-1) ];
    Q_ASSERT( item );

    if (!item)
        kdWarning(1201) << "NOT FOUND   oldUrl=" << oldUrl.prettyURL() << endl;
    else
    {
        // We need to update the URL in m_dictSubDirs
        m_dictSubDirs.remove( oldUrl.url(-1) );
        m_dictSubDirs.insert( newUrl.url(-1), item );
        if (item->parent())
        {
            ((KonqSidebarDirTreeItem*) item)->alias<<oldUrl.url(-1);
            ((KonqSidebarDirTreeItem*) item)->alias<<newUrl.url(-1);
        }
        kdDebug(1201) << "Updating url to " << newUrl.prettyURL() << endl;
    }
}

void KonqSidebarDirTreeModule::slotListingStopped( const KURL & url )
{
    KonqSidebarTreeItem * item = m_dictSubDirs[ url.url(-1) ];

    Q_ASSERT(item);

    kdDebug(1201) << "KonqSidebarDirTree::slotListingStopped " << url.prettyURL() << endl;

    if ( item && item->childCount() == 0 )
    {
        item->setExpandable( false );
        item->repaint();
    }

    kdDebug(1201) << "m_selectAfterOpening " << m_selectAfterOpening.prettyURL() << endl;
    if ( !m_selectAfterOpening.isEmpty() && url.isParentOf( m_selectAfterOpening ) )
    {
        KURL theURL( m_selectAfterOpening );
        m_selectAfterOpening = KURL();
        followURL( theURL );
    }

    m_pTree->stopAnimation( item );
}

void KonqSidebarDirTreeModule::followURL( const KURL & url )
{
    // Check if we already know this URL
    KonqSidebarTreeItem * item = m_dictSubDirs[ url.url(-1) ];
    if (item) // found it  -> ensure visible, select, return.
    {
        m_pTree->ensureItemVisible( item );
        m_pTree->setSelected( item, true );
        return;
    }

    KURL uParent( url );
    KonqSidebarTreeItem * parentItem = 0L;
    // Go up to the first known parent
    do
    {
        uParent = uParent.upURL();
        parentItem = m_dictSubDirs[ uParent.url(-1) ];
    } while ( !parentItem && !uParent.path().isEmpty() && uParent.path() != "/" );

    // Not found !?!
    if (!parentItem)
    {
        kdDebug() << "No parent found for url " << url.prettyURL() << endl;
        return;
    }
    kdDebug(1202) << "Found parent " << uParent.prettyURL() << endl;

    // That's the parent directory we found. Open if not open...
    if ( !parentItem->isOpen() )
    {
        parentItem->setOpen( true );
        if ( parentItem->childCount() && m_dictSubDirs[ url.url(-1) ] )
        {
            // Immediate opening, if the dir was already listed
            followURL( url ); // equivalent to a goto-beginning-of-method
        } else
        {
            m_selectAfterOpening = url;
            kdDebug(1202) << "KonqSidebarDirTreeModule::followURL: m_selectAfterOpening=" << m_selectAfterOpening.url() << endl;
        }
    }
}


extern "C"
{
        KonqSidebarTreeModule *create_konq_sidebartree_dirtree(KonqSidebarTree* par)
	{
		return new KonqSidebarDirTreeModule(par);
	}
}



#include "dirtree_module.moc"
