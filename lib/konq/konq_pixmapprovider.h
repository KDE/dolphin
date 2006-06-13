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

#ifndef KONQ_PIXMAPPROVIDER_H
#define KONQ_PIXMAPPROVIDER_H

#include <kpixmapprovider.h>
#include "konq_faviconmgr.h"

#include <libkonq_export.h>

#include <QMap>
#include <QPixmap>

class KConfigGroup;
class KConfig;

class LIBKONQ_EXPORT KonqPixmapProvider : public KonqFavIconMgr, virtual public KPixmapProvider
{
public:
    static KonqPixmapProvider * self();

    virtual ~KonqPixmapProvider();

    /**
     * Looks up a pixmap for @p url. Uses a cache for the iconname of url.
     */
    virtual QPixmap pixmapFor( const QString& url, int size = 0 );

    /**
     * Loads the cache to @p kc from key @p key.
     */
    void load( KConfigGroup& kc, const QString& key );
    /**
     * Saves the cache to @p kc as key @p key.
     * Only those @p items are saved, otherwise the cache would grow forever.
     */
    void save( KConfigGroup& kc, const QString& key, const QStringList& items );

    /**
     * Clears the pixmap cache
     */
    void clear();

    /**
     * Looks up an iconname for @p url. Uses a cache for the iconname of url.
     */
    QString iconNameFor( const KUrl& url );

protected:
    KonqPixmapProvider();

    /**
     * Overridden from KonqFavIconMgr to update the cache
     */
    virtual void notifyChange( bool isHost, QString hostOrURL, QString iconName );

    QPixmap loadIcon( const QString& url, const QString& icon, int size );

private:
    QMap<KUrl,QString> iconMap;
    static KonqPixmapProvider * s_self;
};


#endif // KONQ_PIXMAPPROVIDER_H
