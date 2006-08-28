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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_pixmapprovider.h"

#include <QBitmap>
#include <QPainter>

#include <kapplication.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kshell.h>
#include <kprotocolinfo.h>

#include <kstaticdeleter.h>

KonqPixmapProvider * KonqPixmapProvider::s_self = 0;
static KStaticDeleter<KonqPixmapProvider> s_konqPixmapProviderSd;

KonqPixmapProvider * KonqPixmapProvider::self()
{
    if ( !s_self )
        s_konqPixmapProviderSd.setObject( s_self, new KonqPixmapProvider );

    return s_self;
}

KonqPixmapProvider::KonqPixmapProvider()
    : KPixmapProvider(),
      KonqFavIconMgr( 0 )
{
}

KonqPixmapProvider::~KonqPixmapProvider()
{
}

// at first, tries to find the iconname in the cache
// if not available, tries to find the pixmap for the mimetype of url
// if that fails, gets the icon for the protocol
// finally, inserts the url/icon pair into the cache
QString KonqPixmapProvider::iconNameFor( const KUrl& url )
{
    QMap<KUrl,QString>::iterator it = iconMap.find( url );
    QString icon;
    if ( it != iconMap.end() ) {
        icon = it.value();
        if ( !icon.isEmpty() )
	    return icon;
    }

    if ( url.url().isEmpty() ) {
        // Use the folder icon for the empty URL
        icon = KMimeType::mimeType( "inode/directory" )->iconName();
        Q_ASSERT( !icon.isEmpty() );
    }
    else
    {
        icon = KMimeType::iconNameForUrl( url );
        Q_ASSERT( !icon.isEmpty() );
    }

    // cache the icon found for url
    iconMap.insert( url, icon );

    return icon;
}

QPixmap KonqPixmapProvider::pixmapFor( const QString& url, int size )
{
    return loadIcon( url, iconNameFor( KUrl( url ) ), size );
}

void KonqPixmapProvider::load( KConfigGroup& kc, const QString& key )
{
    iconMap.clear();
    QStringList list;
    list = kc.readPathListEntry( key );
    QStringList::Iterator it = list.begin();
    QString url, icon;
    while ( it != list.end() ) {
	url = (*it);
	if ( ++it == list.end() )
	    break;
	icon = (*it);
	iconMap.insert( KUrl( url ), icon );

	++it;
    }
}

// only saves the cache for the given list of items to prevent the cache
// from growing forever.
void KonqPixmapProvider::save( KConfigGroup& kc, const QString& key,
			       const QStringList& items )
{
    QStringList list;
    QStringList::ConstIterator it = items.begin();
    QMap<KUrl,QString>::const_iterator mit;
    while ( it != items.end() ) {
	mit = iconMap.find( KUrl(*it) );
	if ( mit != iconMap.end() ) {
	    list.append( mit.key().toString() );
	    list.append( mit.value() );
	}

	++it;
    }
    kc.writePathEntry( key, list );
}

void KonqPixmapProvider::notifyChange( bool isHost, QString hostOrURL,
    QString iconName )
{
    for ( QMap<KUrl,QString>::iterator it = iconMap.begin();
          it != iconMap.end();
          ++it )
    {
        KUrl url( it.key() );
        if ( url.protocol().startsWith("http") &&
            ( ( isHost && url.host() == hostOrURL ) ||
                ( url.host() + url.path() == hostOrURL ) ) )
        {
            // For host default-icons still query the favicon manager to get
            // the correct icon for pages that have an own one.
            QString icon = isHost ? KMimeType::favIconForUrl( url ) : iconName;
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
    if ( size <= K3Icon::SizeSmall )
	return SmallIcon( icon, size );

    KUrl u;
    if ( url.at(0) == '/' )
	u.setPath( url );
    else
	u = url;

    QPixmap big;

    // favicon? => blend the favicon in the large
    if ( url.startsWith( "http:/" ) && icon.startsWith("favicons/") ) {
	QPixmap small = SmallIcon( icon, size );
	big = KGlobal::iconLoader()->loadIcon( KProtocolInfo::icon("http"),
					       K3Icon::Panel, size );

	int x = big.width()  - small.width();
	int y = 0;
#ifdef __GNUC__
#warning This mask merge is probably wrong.  I couldnt find a way to get the old behavior of Qt::OrROP
#endif
/* Old code that did the bitmask merge
       if ( big.mask() ) {
           QBitmap mask = *big.mask();
           bitBlt( &mask, x, y,
            small.mask() ? const_cast<QBitmap *>(small.mask()) : &small, 0, 0,
                   small.width(), small.height(),
                   small.mask() ? OrROP : SetROP );
           big.setMask( mask );
       }
*/
	QBitmap mask( big.mask() );
	if( !mask.isNull() ) {
		QBitmap sm( small.mask() );
        QPainter pt(&mask);
#ifdef __GNUC__
		#warning
#endif
        pt.setCompositionMode( QPainter::CompositionMode_Source );
  pt.drawPixmap(QPoint(x,y), sm.isNull() ? small : QPixmap(sm), QRect(0,0,small.width(), small.height()));
	    big.setMask( mask );
	}
	bitBlt( &big, x, y, &small );
    }

    else // not a favicon..
	big = KGlobal::iconLoader()->loadIcon( icon, K3Icon::Panel, size );

    return big;
}
