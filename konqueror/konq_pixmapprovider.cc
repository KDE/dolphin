/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qstringlist.h>

#include <kconfig.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kprotocolinfo.h>
#include <kurl.h>

#include <konq_faviconmgr.h>

#include "konq_pixmapprovider.h"

// at first, tries to find the iconname in the cache
// if not available, tries to find the pixmap for the mimetype of url
// if that fails, gets the icon for the protocol
// finally, inserts the url/icon pair into the cache
QPixmap KonqPixmapProvider::pixmapFor( const QString& url, int size )
{
    QMapIterator<QString,QString> it = iconMap.find( url );
    QString icon;
    if ( it != iconMap.end() ) {
        icon = it.data();
        if ( !icon.isEmpty() )
	    return KGlobal::iconLoader()->loadIcon(icon, KIcon::Desktop, size);
    }

    KURL u;
    if ( url.at(0) == '/' )
	u.setPath( url );
    else
	u = url;

    icon = KonqFavIconMgr::iconForURL(u.url());
    if (icon.isEmpty())
    {
        KMimeType::Ptr mt = KMimeType::findByURL( u, 0, u.isLocalFile() );
        icon = mt->icon( url, false );
        if ( icon.isEmpty() || icon == QString::fromLatin1( "unknown" ) )
            icon = KProtocolInfo::icon( u.protocol() );
    }
    
    ASSERT( !icon.isEmpty() );
    
    // cache the icon found for url
    iconMap.insert( url, icon );
    
    return KGlobal::iconLoader()->loadIcon( icon, KIcon::Desktop, size );
}


void KonqPixmapProvider::load( KConfig *kc, const QString& key )
{
    iconMap.clear();
    QStringList list;
    list = kc->readListEntry( key );
    QStringList::Iterator it = list.begin();
    QString url, icon;
    while ( it != list.end() ) {
	url = (*it);
	if ( ++it == list.end() )
	    break;
	icon = (*it);
	iconMap.insert( url, icon );
	
	++it;
    }
}

// only saves the cache for the given list of items to prevent the cache
// from growing forever.
void KonqPixmapProvider::save( KConfig *kc, const QString& key, 
			       const QStringList& items )
{
    QStringList list;
    QStringList::ConstIterator it = items.begin();
    QMapConstIterator<QString,QString> mit;
    while ( it != items.end() ) {
	mit = iconMap.find( *it );
	if ( mit != iconMap.end() ) {
	    list.append( mit.key() );
	    list.append( mit.data() );
	}

	++it;
    }
    kc->writeEntry( key, list );
}

void KonqPixmapProvider::updateFavIcons()
{
    for ( QMapIterator<QString,QString> it = iconMap.begin();
          it != iconMap.end();
          ++it )
    {
        QString iconName = KonqFavIconMgr::iconForURL( it.key() );
        if ( ! iconName.isEmpty() )
            *it = iconName;
    }
}

