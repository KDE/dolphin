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

#include <qbitmap.h>
#include <qstringlist.h>

#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kprotocolinfo.h>
#include <kurl.h>

#include "konq_pixmapprovider.h"

KonqPixmapProvider * KonqPixmapProvider::s_self = 0L;

KonqPixmapProvider * KonqPixmapProvider::self()
{
    if ( !s_self )
	s_self = new KonqPixmapProvider( kapp, "KonqPixmapProvider" );

    return s_self;
}

KonqPixmapProvider::KonqPixmapProvider( QObject *parent, const char *name )
    : KonqFavIconMgr( parent, name ),
      KPixmapProvider()
{
}

KonqPixmapProvider::~KonqPixmapProvider()
{
    s_self = 0L;
}

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
	    return loadIcon( url, icon, size );
    }

    KURL u;
    if ( url.at(0) == '/' )
	u.setPath( url );
    else
	u = url;

    icon = KonqFavIconMgr::iconForURL(u.url());
    if ( icon.isEmpty() )
	icon = KMimeType::iconForURL( u );

    ASSERT( !icon.isEmpty() );

    // cache the icon found for url
    iconMap.insert( url, icon );

    return loadIcon( url, icon, size );
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

void KonqPixmapProvider::notifyChange( bool isHost, QString hostOrURL,
    QString iconName )
{
    for ( QMapIterator<QString,QString> it = iconMap.begin();
          it != iconMap.end();
          ++it )
    {
        KURL url( it.key() );
        if ( url.protocol().left( 4 ) == "http" &&
            ( ( isHost && url.host() == hostOrURL ) ||
                ( url.host() + url.path() == hostOrURL ) ) )
        {
            // For host default-icons still query the favicon manager to get
            // the correct icon for pages that have an own one.
            QString icon = isHost ? KonqFavIconMgr::iconForURL( it.key() ) : iconName;
            if ( !icon.isEmpty() )
                *it = icon;
        }
    }

    emit changed();
}

void KonqPixmapProvider::clear()
{
    iconMap.clear();
}

QPixmap KonqPixmapProvider::loadIcon( const QString& url, const QString& icon,
				      int size )
{
    if ( size <= KIcon::SizeSmall )
	return SmallIcon( icon, size );

    KURL u;
    if ( url.at(0) == '/' )
	u.setPath( url );
    else
	u = url;

    QPixmap big;

    // favicon? => blend the favicon in the large
    if ( url.startsWith( "http:/" ) && icon.startsWith("favicons/") ) {
	QPixmap small = SmallIcon( icon, size );
	big = KGlobal::iconLoader()->loadIcon( KProtocolInfo::icon("http"),
					       KIcon::Panel, size );

	int x = big.width()  - small.width();
	int y = 0;

 	if ( big.mask() ) {
 	    QBitmap mask = *big.mask();
 	    bitBlt( &mask, x, y,
            small.mask() ? const_cast<QBitmap *>(small.mask()) : &small, 0, 0,
 		    small.width(), small.height(),
 		    small.mask() ? OrROP : SetROP );
 	    big.setMask( mask );
 	}

	bitBlt( &big, x, y, &small );
    }

    else // not a favicon..
	big = KGlobal::iconLoader()->loadIcon( icon, KIcon::Panel, size );

    return big;
}
