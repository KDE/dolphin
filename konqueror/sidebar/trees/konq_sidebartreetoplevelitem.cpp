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

#include "konq_sidebartreetoplevelitem.h"
//#include "konq_treepart.h"
#include "konq_sidebartree.h"
#include "konq_sidebartreemodule.h"
#include <qstrlist.h>
#include <kdebug.h>
#include <kdirnotify_stub.h>
#include <kglobal.h>
#include <kio/global.h>
#include <kio/paste.h>
#include <konq_operations.h>
#include <kparts/browserextension.h>
#include <kprotocolinfo.h>
#include <ksimpleconfig.h>
#include <kurldrag.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <konq_drag.h>

void KonqSidebarTreeTopLevelItem::init()
{
    QString desktopFile = m_path;
    if ( isTopLevelGroup() )
        desktopFile += "/.directory";
    KSimpleConfig cfg( desktopFile, true );
    cfg.setDesktopGroup();
    m_comment = cfg.readEntry( "Comment" );
}

void KonqSidebarTreeTopLevelItem::setOpen( bool open )
{
    if (open && module())
        module()->openTopLevelItem( this );
    KonqSidebarTreeItem::setOpen( open );
}

void KonqSidebarTreeTopLevelItem::itemSelected()
{
    kdDebug() << "KonqSidebarTreeTopLevelItem::itemSelected" << endl;
    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = m_bTopLevelGroup && data->provides("text/uri-list");
    tree()->part()->enableActions( true, true, paste, true, true, true,
                                                true /*rename*/ );
}

bool KonqSidebarTreeTopLevelItem::acceptsDrops( const QStrList & formats )
{
    return formats.contains("text/uri-list") &&
        ( m_bTopLevelGroup || !externalURL().isEmpty() );
}

void KonqSidebarTreeTopLevelItem::drop( QDropEvent * ev )
{
    if ( m_bTopLevelGroup )
    {
        // When dropping something to "Network" or its subdirs, we want to create
        // a desktop link, not to move/copy/link - except for .desktop files :-}
        KURL::List lst;
        if ( KURLDrag::decode( ev, lst ) && !lst.isEmpty() ) // Are they urls ?
        {
            // hack - we should check every file, but doDrop takes a dropevent...
            if ( lst.first().fileName().right(8) == ".desktop" )
            {
                KURL destURL;
                destURL.setPath( m_path );
                KonqOperations::doDrop( 0L, destURL, ev, tree() );
            }
            else
            {
                KURL::List::Iterator it = lst.begin();
                for ( ; it != lst.end() ; it++ )
                {
                    const KURL & targetURL = (*it);
                    KURL linkURL;
                    linkURL.setPath( m_path );
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
            }
        } else
            kdError(1202) << "No URL !?  " << endl;
    }
    else // Top level item, not group
    {
        if ( !externalURL().isEmpty() )
            KonqOperations::doDrop( 0L, externalURL(), ev, tree() );
    }
}

QDragObject * KonqSidebarTreeTopLevelItem::dragObject( QWidget * parent, bool move )
{
    // 100% duplicated from KonqDirTreeItem::dragObject :(
    KURL::List lst;
    KURL url;
    url.setPath( path() );
    lst.append( url );

    KonqDrag * drag = KonqDrag::newDrag( lst, false, parent );

    const QPixmap * pix = pixmap(0);
    if (pix)
    {
        QPoint hotspot( pix->width() / 2, pix->height() / 2 );
        drag->setPixmap( *pix, hotspot );
    }
    drag->setMoveSelection( move );

    return drag;
}

void KonqSidebarTreeTopLevelItem::middleButtonPressed()
{
    if ( !m_bTopLevelGroup )
        emit tree()->part()->getInterfaces()->getExtension()->createNewWindow( m_externalURL );
    // Do nothing for toplevel groups
}

void KonqSidebarTreeTopLevelItem::rightButtonPressed()
{
    KURL url;
    url.setPath( m_path );
    // We don't show "edit file type" (useless here) and "properties" (shows the wrong name,
    // i.e. the filename instead of the Name field). There's the Rename item for that.
    // Only missing thing is changing the URL of a link. Hmm...
    emit tree()->part()->getInterfaces()->getExtension()->popupMenu( QCursor::pos(), url,
                                                 isTopLevelGroup() ? "inode/directory" : "application/x-desktop" );
}


void KonqSidebarTreeTopLevelItem::trash()
{
    delOperation( KonqOperations::TRASH );
}

void KonqSidebarTreeTopLevelItem::del()
{
    delOperation( KonqOperations::DEL );
}

void KonqSidebarTreeTopLevelItem::shred()
{
    delOperation( KonqOperations::SHRED );
}

void KonqSidebarTreeTopLevelItem::delOperation( int method )
{
    KURL url;
    url.setPath( m_path );
    KURL::List lst;
    lst.append(url);

    KonqOperations::del(tree(), method, lst);
}

void KonqSidebarTreeTopLevelItem::paste()
{
    // move or not move ?
    bool move = false;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) ) {
        move = KonqDrag::decodeIsCutSelection( data );
        kdDebug(1201) << "move (from clipboard data) = " << move << endl;
    }

    KURL destURL;
    if ( m_bTopLevelGroup )
        destURL.setPath( m_path );
    else
        destURL = m_externalURL;

    KIO::pasteClipboard( destURL, move );
}

void KonqSidebarTreeTopLevelItem::rename()
{
    tree()->rename( this, 0 );
}

void KonqSidebarTreeTopLevelItem::rename( const QString & name )
{
    KURL url;
    url.setPath( m_path );

    // Well, it's not really the file we want to rename, it's the Name field
    // of the .directory or desktop file
    //KonqOperations::rename( tree(), url, name );

    QString desktopFile = m_path;
    if ( isTopLevelGroup() )
        desktopFile += "/.directory";
    KSimpleConfig cfg( desktopFile );
    cfg.setDesktopGroup();
    cfg.writeEntry( "Name", name );
    cfg.sync();

    // Notify about the change
    KURL::List lst;
    lst.append(url);
    KDirNotify_stub allDirNotify("*", "KDirNotify*");
    allDirNotify.FilesChanged( lst );
}

QString KonqSidebarTreeTopLevelItem::toolTipText() const
{
    return m_comment;
}

