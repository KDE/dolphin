/* This file is part of the KDE project
   Copyright (C) 2002 Lubos Lunak <l.lunak@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "preloader.h"

#include <kconfig.h>
#include <dcopref.h>
#include <kapplication.h>
#include <dcopclient.h>

KonqyPreloader::KonqyPreloader( const QCString& obj )
    : KDEDModule( obj )
    {
    reconfigure();
    connect( kapp->dcopClient(), SIGNAL( applicationRemoved( const QCString& )),
        SLOT( appRemoved( const QCString& )));
    }

KonqyPreloader::~KonqyPreloader()
    {
    max_count = 0;
    reduceCount();
    }

bool KonqyPreloader::registerPreloadedKonqy( QCString id )
    {
    if( instances.count() >= max_count )
        return false;
    instances.append( KonqyData( id ));
    return true;
    }

QCString KonqyPreloader::getPreloadedKonqy()
    {
    if( instances.count() == 0 )
        return "";
    KonqyData konqy = instances.first();
    instances.pop_front();
    return konqy.id;
    }
    
void KonqyPreloader::unregisterPreloadedKonqy( QCString id_P )
    {
    for( InstancesList::Iterator it = instances.begin();
         it != instances.end();
         ++it )
        if( (*it).id == id_P )
            {
            instances.remove( it );
            return;
            }
    }

void KonqyPreloader::appRemoved( const QCString& id )
    {
    unregisterPreloadedKonqy( id );
    }
    
void KonqyPreloader::reconfigure()
    {
    KConfig cfg( QString::fromLatin1( "konquerorrc" ), true );
    KConfigGroupSaver group( &cfg, "Reusing" );
    max_count = cfg.readNumEntry( "MaxPreloadCount", 0 );
    reduceCount();
    }

void KonqyPreloader::reduceCount()
    {
    while( instances.count() > max_count )
        {
        KonqyData konqy = instances.first();
        instances.pop_front();
        DCOPRef ref( konqy.id, "KonquerorIface" );
        ref.send( "terminatePreloaded" );
        }
    }

extern "C"
KDEDModule *create_konqy_preloader( const QCString& obj )
    {
    return new KonqyPreloader( obj );
    }

#include "preloader.moc"
