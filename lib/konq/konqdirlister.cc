/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "konqdirlister.h"
#include "konqfileitem.h"
#include <kdebug.h>

KonqDirLister::KonqDirLister( bool _delayedMimeTypes )
  : KDirLister( _delayedMimeTypes )
{
  //m_bKofficeDocs = false;
}

KonqDirLister::~KonqDirLister()
{
}

KFileItem * KonqDirLister::createFileItem( const KIO::UDSEntry& entry,
					   const KURL& url,
					   bool determineMimeTypeOnDemand,
					   bool isDirectory )
{
    /*
       // Detect koffice files
       QString mimeType = item->name();
       if ( mimeType.left(15) == "application/x-k" )
       {
       // Currently this matches all koffice mimetypes
       // To be changed later on if anybody else uses a x-k* mimetype
       m_bKofficeDocs = true;
       }
    */
    return new KonqFileItem( entry, url, determineMimeTypeOnDemand, isDirectory );
}

#include "konqdirlister.moc"
