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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//#include "konq_treepart.h"
#include "konq_sidebartreemodule.h"
#include <kdebug.h>
#include <kdirnotify_stub.h>
#include <kio/paste.h>
#include <konq_operations.h>
#include <kprotocolinfo.h>
#include <k3urldrag.h>
#include <kmimetype.h>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <qmimedata.h>
#include <konqmimedata.h>

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
    kDebug() << "KonqSidebarTreeTopLevelItem::itemSelected" << endl;
    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = m_bTopLevelGroup && data->provides("text/uri-list");
    tree()->enableActions( true, true, paste, true, true, true /*rename*/ );
}

bool KonqSidebarTreeTopLevelItem::acceptsDrops( const Q3StrList & formats )
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
        KUrl::List lst;
        if ( K3URLDrag::decode( ev, lst ) && !lst.isEmpty() ) // Are they urls ?
        {
            KUrl::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
            {
                tree()->addUrl(this, *it);
            }
        } else
            kError(1202) << "No URL !?  " << endl;
    }
    else // Top level item, not group
    {
        if ( !externalURL().isEmpty() )
            KonqOperations::doDrop( 0L, externalURL(), ev, tree() );
    }
}

bool KonqSidebarTreeTopLevelItem::populateMimeData( QMimeData* mimeData, bool move )
{
    // 100% duplicated from KonqDirTreeItem::dragObject :(
    KUrl::List lst;
    KUrl url;
    url.setPath( path() );
    lst.append( url );

    KonqMimeData::populateMimeData( mimeData, KUrl::List(), lst, move );

#if 0 // was this ever used? Seems populateMimeData is only used for copy/cut, not for dragging?
    const QPixmap * pix = pixmap(0);
    if (pix)
    {
        QPoint hotspot( pix->width() / 2, pix->height() / 2 );
        drag->setPixmap( *pix, hotspot );
    }
#endif

    return true;
}

void KonqSidebarTreeTopLevelItem::middleButtonClicked()
{
    if ( !m_bTopLevelGroup )
        emit tree()->createNewWindow( m_externalURL );
    // Do nothing for toplevel groups
}

void KonqSidebarTreeTopLevelItem::rightButtonPressed()
{
    KUrl url;
    url.setPath( m_path );
    // We don't show "edit file type" (useless here) and "properties" (shows the wrong name,
    // i.e. the filename instead of the Name field). There's the Rename item for that.
    // Only missing thing is changing the URL of a link. Hmm...

    if ( !module() || !module()->handleTopLevelContextMenu( this, QCursor::pos() ) )
    {
        tree()->showToplevelContextMenu();
    }
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
    KUrl url;
    url.setPath( m_path );
    KUrl::List lst;
    lst.append(url);

    KonqOperations::del(tree(), method, lst);
}

void KonqSidebarTreeTopLevelItem::paste()
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) ) {
        move = KonqMimeData::decodeIsCutSelection( data );
        kDebug(1201) << "move (from clipboard data) = " << move << endl;
    }

    KUrl destURL;
    if ( m_bTopLevelGroup )
        destURL.setPath( m_path );
    else
        destURL = m_externalURL;

    KIO::pasteClipboard( destURL, 0L,move );
}

void KonqSidebarTreeTopLevelItem::rename()
{
    tree()->rename( this, 0 );
}

void KonqSidebarTreeTopLevelItem::rename( const QString & name )
{
    KUrl url;
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
    KUrl::List lst;
    lst.append(url);
    KDirNotify_stub allDirNotify("*", "KDirNotify*");
    allDirNotify.FilesChanged( lst );
}

QString KonqSidebarTreeTopLevelItem::toolTipText() const
{
    return m_comment;
}

