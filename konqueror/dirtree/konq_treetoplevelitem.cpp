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

#include "konq_treetoplevelitem.h"
#include "konq_treepart.h"
#include "konq_tree.h"
#include <qstrlist.h>
#include <kdebug.h>
#include <kdirnotify_stub.h>
#include <kglobal.h>
#include <kio/global.h>
#include <konq_operations.h>
#include <kparts/browserextension.h>
#include <kprotocolinfo.h>
#include <ksimpleconfig.h>
#include <kurldrag.h>
#include <qapplication.h>
#include <qclipboard.h>

void KonqTreeTopLevelItem::itemSelected()
{
    KParts::BrowserExtension * ext = tree()->part()->extension();
    emit ext->enableAction( "copy", true );
    emit ext->enableAction( "cut", true );
    emit ext->enableAction( "trash", true );
    emit ext->enableAction( "del", true );
    emit ext->enableAction( "shred", true );

    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = m_bTopLevelGroup && data->provides("text/uri-list");
    emit ext->enableAction( "paste", paste );
}

bool KonqTreeTopLevelItem::acceptsDrops( const QStrList & formats )
{
    return formats.contains("text/uri-list");
}

void KonqTreeTopLevelItem::drop( QDropEvent * ev )
{
    if ( m_bTopLevelGroup )
    {
        // When dropping something to "Network" or its subdirs, we want to create
        // a desktop link, not to move/copy/link
        KURL::List lst;
        if ( KURLDrag::decode( ev, lst ) ) // Are they urls ?
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
        } else
            kdError(1202) << "No URL !?  " << endl;
    }
    else // Top level item, not group
    {
        KonqOperations::doDrop( 0L, externalURL(), ev, tree() );
    }
}

void KonqTreeTopLevelItem::middleButtonPressed()
{
    emit tree()->part()->extension()->openURLRequest( externalURL() );
}

void KonqTreeTopLevelItem::rightButtonPressed()
{
    KURL url;
    url.setPath( m_path );
    emit tree()->part()->extension()->popupMenu( QCursor::pos(), url,
                                                 isTopLevelGroup() ? "inode/directory" : "application/x-desktop" );
}
