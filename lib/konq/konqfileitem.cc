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
    QString xvpicPath = m_url.directory() +
                  // Append .xvpics if not already in an .xvpics dir
                  ((KURL(m_url.directory()).filename(true) != ".xvpics") ? "/.xvpics/" : "/");
    xvpicPath += m_url.filename();
    QPixmap pix;

    // Is the xv pic available ?
    struct stat buff;
    bool bAvail = false;
    if ( stat( xvpicPath.local8Bit(), &buff ) == 0 )
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
      if ( pix.load( xvpicPath ) )
        return pix;
    } else
    {
      // No xv pic, or too old -> load the orig image and create the XV pic
      if ( pix.load( m_url.path() ) )
      {
        bool bCanSave = true;
        // Create .xvpics/ if it doesn't exist
        QDir xvDir( m_url.directory() );
        if ( !xvDir.exists(".xvpics") )
        {
          bCanSave = xvDir.mkdir(".xvpics");
        }
        // Save XV file
        int w, h;
        if ( pix.width() > pix.height() )
        {
          w = QMIN( pix.width(), 80 ); // TODO make configurable for tackat :-)
          h = (int)( (float)pix.height() * ( (float)w / (float)pix.width() ) );
        }
        else
        {
          h = QMIN( pix.height(), 60 ); // TODO make configurable for tackat :-)
          w = (int)( (float)pix.width() * ( (float)h / (float)pix.height() ) );
        }
        if (bCanSave)
        {
          QImageIO iio;
          iio.setImage( pix.convertToImage().smoothScale( w, h ) );
          iio.setFileName( xvpicPath );
          iio.setFormat( "XV" );
          bCanSave = iio.write();
          // Load it
          if ( pix.load( xvpicPath ) ) return pix;
        }
        if (!bCanSave) // not "else", write may have failed !
        {
          // Ok, this is ugly and slow. Anybody knows of a better solution ?
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
      if (mySize < 1024)
        text = i18n("%1 (%2 bytes)").arg(text2).arg((long) mySize);
      else
      {
	float d = (float) mySize/1024.0;
        text = i18n("%1 (%2 KB)").arg(text2).arg(KGlobal::locale()->formatNumber(d, 2));
      }
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

