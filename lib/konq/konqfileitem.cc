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
      kdDebug(1203) << "Requested thumbnail size: " << _size << endl;
      if(_size == 0) // default size requested
      {
          KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
          _size = root->defaultSize( KIcon::Desktop );
      }
      struct stat buff;
      bool bAvail = false;
      QString thumbPath;
      QPixmap pix;

      // Check if pixie thumbnail of any size is there first, preferring
      // the pixie equivalent to the requested icon size
      if(_size < 28){
          thumbPath = m_url.directory() + "/.mospics/small/" + m_url.filename();
          bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          if(!bAvail){
              thumbPath = m_url.directory() + "/.mospics/med/" + m_url.filename();
              bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          }
          if(!bAvail){
              thumbPath = m_url.directory() + "/.mospics/large/" + m_url.filename();
              bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          }
      }
      else if(_size < 40){
          thumbPath = m_url.directory() + "/.mospics/med/" + m_url.filename();
          bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          if(!bAvail){
              thumbPath = m_url.directory() + "/.mospics/small/" + m_url.filename();
              bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          }
          if(!bAvail){
              thumbPath = m_url.directory() + "/.mospics/large/" + m_url.filename();
              bAvail = stat(thumbPath.local8Bit(), &buff) == 0;
          }
      }
      else{
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
              if (pix.load(thumbPath))
                  return pix;
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
          int extent;
          QString sizeStr;
          if(_size < 28){
              thumbPath = m_url.directory() + "/.mospics/small/" + m_url.filename();
              extent = 48;
              sizeStr = ".mospics/small";
          }
          else if(_size < 40){
              thumbPath = m_url.directory() + "/.mospics/med/" + m_url.filename();
              extent = 64;
              sizeStr = ".mospics/med";
          }
          else{
              thumbPath = m_url.directory() + "/.mospics/large/" + m_url.filename();
              extent = 90;
              sizeStr = ".mospics/large";
          }
          if (pix.load(m_url.path()))
          {
              bool bCanSave = true;
              // Create .mospics/large if it doesn't exist
              QDir mosDir(m_url.directory());
              if (!mosDir.exists(".mospics"))
              {
                  bCanSave = mosDir.mkdir(".mospics");
              }
              if (!mosDir.exists(sizeStr))
              {
                  bCanSave = mosDir.mkdir(sizeStr);
              }
              int w = pix.width(), h = pix.height();
              // scale to pixie size
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
              }
              QImage img = pix.convertToImage().smoothScale( w, h );
              pix.convertFromImage( img );
              if (bCanSave)
              {
                  // write
                  QImageIO iio;
                  iio.setImage(img);
                  iio.setFileName( thumbPath );
                  iio.setFormat("PNG");
                  bCanSave = iio.write();
                  return pix;
              }
          }
      }
  }

  QPixmap p = m_pMimeType->pixmap( m_url, KIcon::Desktop, _size );
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

