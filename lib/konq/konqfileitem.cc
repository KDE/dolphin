/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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
// $Id$

#include <sys/time.h>
#include <unistd.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

#include "konqfileitem.h"

#include <qdir.h>
#include <qimage.h>
#include <qpixmap.h>

#include <kdebug.h>
#include <kmimetype.h>

QPixmap KonqFileItem::pixmap( int _size, int _state,
	                      bool bImagePreviewAllowed ) const
{
  bThumbnail = false;

  if ( m_pMimeType && m_pMimeType->name().left(6) == "image/" &&
       bImagePreviewAllowed )
  {
      kdDebug(1203) << "Requested thumbnail size: " << _size << endl;
      if(_size == 0) // default size requested
      {
          KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
          _size = root->defaultSize( KIcon::Desktop );
      }
      KURL thumbURL( m_url );
      QPixmap pix;
      KIO::UDSEntry entry;

      // Check if pixie thumbnail of any size is there first, preferring
      // the pixie equivalent to the requested icon size
      if(_size < 28){
          thumbURL.setPath( m_url.directory() + "/.mospics/small/" + m_url.fileName() );
          bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/med/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/large/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
      }
      else if(_size < 40){
          thumbURL.setPath( m_url.directory() + "/.mospics/med/" + m_url.fileName() );
          bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/small/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/large/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
      }
      else{
          thumbURL.setPath( m_url.directory() + "/.mospics/large/" + m_url.fileName() );
          bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/med/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
          if(!bThumbnail){
              thumbURL.setPath( m_url.directory() + "/.mospics/small/" + m_url.fileName() );
              bThumbnail = KIO::NetAccess::stat(thumbURL, entry);
          }
      }

      // Get time of the orig file
      time_t tOrig = 0L;
      if ( KIO::NetAccess::stat( m_url.path(), entry ) ){
        KIO::UDSEntry::ConstIterator it = entry.begin();
        for( ; it != entry.end(); it++ ) {
          if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
            tOrig = (time_t)((*it).m_long);
          }
        }
      } // else... well we have a problem, Houston: no more orig file

      if(bThumbnail){

          // Get time of thumbnail file
          time_t t1 = 0L;
          KIO::UDSEntry::ConstIterator it = entry.begin();
          for( ; it != entry.end(); it++ ) {
            if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
              t1 = (time_t)((*it).m_long);
            }
          }

          // Is the thumbnail outdated ?
          if ( t1 < tOrig )
                bThumbnail = false;
      }

      if (!bThumbnail) {
        // check to see if there is an existing Xv thumbnail

        thumbURL.setPath( m_url.directory() +    // base dir
           ((KURL(m_url.directory()).fileName(true) != ".xvpics")
               ? "/.xvpics/" : "/")              // .xvpics if not already there
           + m_url.fileName() );                 // file name

        // Is the xv pic available ?
        if ( KIO::NetAccess::stat( thumbURL, entry ) )
        {
          bThumbnail = true;
          // Get time of thumbnail file
          time_t t1 = 0L;
          KIO::UDSEntry::ConstIterator it = entry.begin();
          for( ; it != entry.end(); it++ ) {
            if ( (*it).m_uds == KIO::UDS_MODIFICATION_TIME ) {
              t1 = (time_t)((*it).m_long);
            }
          }

          // Is the thumbnail outdated ?
          if ( t1 < tOrig )
            bThumbnail = false;
        }
      }

      if(bThumbnail) {
          // Load thumbnail
          QString tmpFile;
          if ( KIO::NetAccess::download( thumbURL, tmpFile )
               && pix.load(tmpFile) )
          {
              KIO::NetAccess::removeTempFile( tmpFile );
              return pix;
          }
          else
              bThumbnail = false;
      }

      // No thumbnail, or too old -> load the orig image and create Pixie pic
      int extent;
      QString sizeStr;
      KURL grandFather( KURL( m_url.directory() ).directory() );
      QString mospicsPath = (grandFather.fileName() == ".mospics") ? grandFather.path() : m_url.directory();
      if(_size < 28){
          thumbURL.setPath( mospicsPath + "/.mospics/small/" + m_url.fileName() );
          extent = 48;
          sizeStr = "/.mospics/small";
      }
      else if(_size < 40){
          thumbURL.setPath( mospicsPath + "/.mospics/med/" + m_url.fileName() );
          extent = 64;
          sizeStr = "/.mospics/med";
      }
      else{
          thumbURL.setPath( mospicsPath + "/.mospics/large/" + m_url.fileName() );
          extent = 90;
          sizeStr = "/.mospics/large";
      }

      QString tmpFile;
      if ( KIO::NetAccess::download( m_url, tmpFile )
               && pix.load(tmpFile) )
      {
          KIO::NetAccess::removeTempFile( tmpFile );
          bool bCanSave = true;
          // Create .mospics/large if it doesn't exist
          KURL dirURL( m_url );
          dirURL.setPath( m_url.directory() + "/.mospics" );
          if (!KIO::NetAccess::exists( dirURL ))
          {
              bCanSave = KIO::NetAccess::mkdir( dirURL );
          }
          dirURL.setPath( m_url.directory() + sizeStr );
          if (bCanSave && !KIO::NetAccess::exists( dirURL ))
          {
              bCanSave = KIO::NetAccess::mkdir( dirURL );
          }
          int w = pix.width(), h = pix.height();
          // scale to pixie size
          QImage img;
          if(pix.width() > extent || pix.height() > extent){
              if(pix.width() > pix.height()){
                  float percent = (((float)extent)/pix.width());
                  h = (int)(pix.height()*percent);
                  w = extent;
              }
              else{
                  float percent = (((float)extent)/pix.height());
                  w = (int)(pix.width()*percent);
                  h = extent;
              }
              img = pix.convertToImage().smoothScale( w, h );
              pix.convertFromImage( img );
          }
          else if(bCanSave)
              img = pix.convertToImage();

          if (bCanSave)
          {
              // write
              QString tmpFile;
              if ( thumbURL.isLocalFile() )
              {
                  tmpFile = thumbURL.path();
              } else {
                  KTempFile temp;
                  if (temp.status() != 0)
                      return pix;
                  tmpFile = temp.name();
              }
              QImageIO iio;
              iio.setImage(img);
              iio.setFileName( tmpFile );
              iio.setFormat("PNG");
              if ( iio.write() )
                  if ( !thumbURL.isLocalFile() )
                  {
                      KIO::NetAccess::upload( tmpFile, thumbURL );
                      (void) ::unlink( tmpFile );
                  }
          }
          return pix;
      }
  }

  return KFileItem::pixmap( _size, _state );
}
