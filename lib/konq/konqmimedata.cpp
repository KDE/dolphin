/* This file is part of the KDE projects
   Copyright (C) 2005 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqmimedata.h"
#include <qmimedata.h>
#include <kdebug.h>

void KonqMimeData::populateMimeData( QMimeData* mimeData,
                                     const KURL::List& kdeURLs,
                                     const KURL::List& mostLocalURLs,
                                     bool cut )
{
    mostLocalURLs.populateMimeData( mimeData );

    // Mostly copied from KURL::List::populateMimeData
    if ( !kdeURLs.isEmpty() )
    {
        QList<QByteArray> urlStringList;
        KURL::List::ConstIterator uit = kdeURLs.begin();
        const KURL::List::ConstIterator uEnd = kdeURLs.end();
        for ( ; uit != uEnd ; ++uit )
        {
            // Get each URL encoded in toUtf8 - and since we get it in escaped
            // form on top of that, .toLatin1() is fine.
            urlStringList.append( (*uit).toMimeDataString().toLatin1() );
        }

        QByteArray uriListData;
        for ( QList<QByteArray>::const_iterator it = urlStringList.begin(), end = urlStringList.end()
                                                     ; it != end ; ++it ) {
            uriListData += (*it);
            uriListData += "\r\n";
        }
        mimeData->setData( "text/uri-list", uriListData );
    }

    QByteArray cutSelectionData = cut ? "1" : "0";
    mimeData->setData( "application/x-kde-cutselection", cutSelectionData );
}

bool KonqMimeData::decodeIsCutSelection( const QMimeData *mimeData )
{
  QByteArray a = mimeData->data( "application/x-kde-cutselection" );
  if ( a.isEmpty() )
    return false;
  else
  {
    kdDebug(1203) << "KonqDrag::decodeIsCutSelection : a=" << a << endl;
    return (a.at(0) == '1'); // true if 1
  }
}
