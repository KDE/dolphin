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

#include <assert.h>
#include <unistd.h>

#include "konqfileitem.h"

#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpixmap.h>

#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmimetype.h>
#include <krun.h>

QPixmap KonqFileItem::pixmap( int _size, bool bImagePreviewAllowed ) const
{
  if ( !m_pMimeType )
  {
    if ( S_ISDIR( m_fileMode ) )
     return DesktopIcon( "folder", _size );

    return DesktopIcon( "unknown", _size );
  }


  if ( m_pMimeType->name().left(6) == "image/" && m_bIsLocalURL && bImagePreviewAllowed )
  {
      warning("Requested thumbnail size: %d", _size);
      if(_size == 0) // hack!
          _size = 48;
      // Check if pixie thumbnail of any size is there first
      struct stat buff;
      bool bAvail = false;
      QString thumbPath;
      QPixmap pix;

      thumbPath = m_url.directory() + "/.mospics/large/" + m_url.filename();
      bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
      if(!bAvail){
          thumbPath = m_url.directory() + "/.mospics/med/" + m_url.filename();
          bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
      }
      if(!bAvail){
          thumbPath = m_url.directory() + "/.mospics/small/" + m_url.filename();
          bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
      }
      if(bAvail){
          time_t t1 = buff.st_mtime;
          // Get time of the orig file
          if ( lstat( m_url.path().local8Bit(), &buff ) == 0 ){
              time_t t2 = buff.st_mtime;
              // Is it outdated ?
              if ( t1 < t2 )
                  bAvail = false;
          }
          if(bAvail){
              if (pix.load(thumbPath)){
                  int w = pix.width(), h = pix.height();
                  if(pix.width() > _size || pix.height() > _size){
                      if(pix.width() > pix.height()){
                          float percent = (((float)_size)/pix.width());
                          h = (int)(pix.height()*percent);
                          w = _size;
                      }
                      else{
                          float percent = (((float)_size)/pix.height());
                          w = (int)(pix.width()*percent);
                          h = _size;
                      }
                      QImage img = pix.convertToImage().smoothScale( w, h );
                      pix.convertFromImage( img );
                  }
                  return pix;
              }
              else
                  bAvail = false;
          }
      }

      // check to see if there is an existing Xv thumbnail

      thumbPath = m_url.directory() +
          ((KURL(m_url.directory()).filename(true) != ".xvpics") ? "/.xvpics/" : "/");
      thumbPath += m_url.filename();

      // Is the xv pic available ?
      if ( stat( thumbPath.local8Bit(), &buff ) == 0 )
      { // Yes
          bAvail = true;
          // Get the time of the xv pic
          time_t t1 = buff.st_mtime;
          // Get time of the orig file
          if ( lstat( m_url.path().local8Bit(), &buff ) == 0 )
          {
              time_t t2 = buff.st_mtime;
              // Is it outdated ?
              if ( t1 < t2 )
                  bAvail = false;
          }
      }

      if ( bAvail )
      {
          if (pix.load( thumbPath ))
              return pix;
      }
      else{
          // No xv pic, or too old -> load the orig image and create Pixie pic
          thumbPath = m_url.directory() + "/.mospics/large/" + m_url.filename();
          if (pix.load(m_url.path()))
          {
              bool bCanSave = true;
              // Create .mospics/large if it doesn't exist
              QDir mosDir(m_url.directory());
              if (!mosDir.exists(".mospics"))
              {
                  bCanSave = mosDir.mkdir(".mospics");
              }
              if (!mosDir.exists(".mospics/large"))
              {
                  bCanSave = mosDir.mkdir(".mospics/large");
              }

              if (bCanSave)
              {
                  // Save large PNG file
                  int w = pix.width(), h = pix.height();
                  // scale to pixie size
                  if(pix.width() > 90 || pix.height() > 90){
                      if(pix.width() > pix.height()){
                          float percent = (((float)90)/pix.width());
                          h = (int)(pix.height()*percent);
                          w = 90;
                      }
                      else{
                          float percent = (((float)90)/pix.height());
                          w = (int)(pix.width()*percent);
                          h = 90;
                      }
                  }
                  // write
                  QImageIO iio;
                  iio.setImage( pix.convertToImage().smoothScale( w, h ) );
                  iio.setFileName( thumbPath );
                  iio.setFormat("PNG");
                  bCanSave = iio.write();
                  // scale again to screen size if needed
                  w = pix.width(), h = pix.height();
                  if(pix.width() > _size || pix.height() > _size){
                      if(pix.width() > pix.height()){
                          float percent = (((float)_size)/pix.width());
                          h = (int)(pix.height()*percent);
                          w = _size;
                      }
                      else{
                          float percent = (((float)_size)/pix.height());
                          w = (int)(pix.width()*percent);
                          h = _size;
                      }
                      QImage img = pix.convertToImage().smoothScale( w, h );
                      pix.convertFromImage( img );
                  }
                  return pix;
              }
              else{
                  int w = pix.width(), h = pix.height();
                  if(pix.width() > _size || pix.height() > _size){
                      if(pix.width() > pix.height()){
                          float percent = (((float)_size)/pix.width());
                          h = (int)(pix.height()*percent);
                          w = _size;
                      }
                      else{
                          float percent = (((float)_size)/pix.height());
                          w = (int)(pix.width()*percent);
                          h = _size;
                      }
                  }
                  QImage img = pix.convertToImage().smoothScale( w, h );
                  pix.convertFromImage( img );
                  return pix;
              }
          }
      }
  }

  QPixmap p = m_pMimeType->pixmap( m_url, _size );
  if (p.isNull())
    warning("Pixmap not found for mimetype %s",m_pMimeType->name().latin1());
  return p;
}

bool KonqFileItem::acceptsDrops()
{
  // Any directory : yes
  if ( S_ISDIR( mode() ) )
    return true;

  // But only local .desktop files and executables
  if ( !m_bIsLocalURL )
    return false;

  if ( m_pMimeType && mimetype() == "application/x-desktop")
    return true;

  // Executable, shell script ... ?
  if ( access( m_url.path(), X_OK ) == 0 )
    return true;

  return false;
}

QString KonqFileItem::getStatusBarInfo()
{
  QString comment = determineMimeType()->comment( m_url, false );
  QString text = m_strText;
  // Extract from the KIO::UDSEntry the additional info we didn't get previously
  QString myLinkDest = linkDest();
  long mySize = size();

  QString text2 = text.copy();

  if ( m_bLink )
  {
      QString tmp;
      if ( comment.isEmpty() )
	tmp = i18n ( "Symbolic Link" );
      else
        tmp = i18n("%1 (Link)").arg(comment);
      text += "->";
      text += myLinkDest;
      text += "  ";
      text += tmp;
  }
  else if ( S_ISREG( m_fileMode ) )
  {
      text = QString("%1 (%2)").arg(text2).arg( KIO::convertSize( mySize ) );
      text += "  ";
      text += comment;
  }
  else if ( S_ISDIR ( m_fileMode ) )
  {
      text += "/  ";
      text += comment;
    }
    else
    {
      text += "  ";
      text += comment;
    }	
    return text;
}

void KonqFileItem::run()
{
  (void) new KRun( m_url, m_fileMode, m_bIsLocalURL );
}

/* Doesn't deserve a method
QString KonqFileItem::makeTimeString( time_t _time )
{
  QDateTime dt;
  dt.setTime_t(_time);

  return KGlobal::locale()->formatDateTime(dt);
}
*/

