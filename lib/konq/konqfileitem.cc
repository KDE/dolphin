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

KonqFileItem::KonqFileItem( const KIO::UDSEntry& _entry, KURL& _url, bool _determineMimeTypeOnDemand ) :
  m_entry( _entry ),
  m_url( _url ),
  m_bIsLocalURL( _url.isLocalFile() ),
  m_fileMode( (mode_t)-1 ),
  m_permissions( (mode_t)-1 ),
  m_bLink( false ),
  m_pMimeType( 0 ),
  m_bMarked( false )
{
  // extract the mode and the filename from the KIO::UDS Entry
  KIO::UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ ) {
  switch ((*it).m_uds) {

    case KIO::UDS_FILE_TYPE:
      m_fileMode = (mode_t)((*it).m_long);
      break;

    case KIO::UDS_ACCESS:
      m_permissions = (mode_t)((*it).m_long);
      break;

    case KIO::UDS_USER:
      m_user = ((*it).m_str);
      break;

    case KIO::UDS_GROUP:
      m_group = ((*it).m_str);
      break;

    case KIO::UDS_NAME:
      m_strText = KIO::decodeFileName( (*it).m_str );
      break;

    case KIO::UDS_URL:
      m_url = KURL((*it).m_str);
      break;

    case KIO::UDS_MIME_TYPE:
      m_pMimeType = KMimeType::mimeType((*it).m_str);
      break;

    case KIO::UDS_LINK_DEST:
      m_bLink = !(*it).m_str.isEmpty(); // we don't store the link dest
      break;
  }
  }
  init( _determineMimeTypeOnDemand );
}

KonqFileItem::KonqFileItem( mode_t _mode, mode_t _permissions, const KURL& _url, bool _determineMimeTypeOnDemand ) :
  m_entry(), // warning !
  m_url( _url ),
  m_bIsLocalURL( _url.isLocalFile() ),
  m_strText( KIO::decodeFileName( _url.filename() ) ),
  m_fileMode ( _mode ),
  m_permissions( _permissions ),
  m_bLink( false ),
  m_bMarked( false )
{
  init( _determineMimeTypeOnDemand );
}

KonqFileItem::KonqFileItem( const KURL &url, const QString &mimeType, mode_t mode )
:  m_url( url ),
  m_bIsLocalURL( url.isLocalFile() ),
  m_strText( KIO::decodeFileName( url.filename() ) ),
  m_fileMode( mode ),
  m_permissions( 0 ),
  m_bLink( false ),
  m_bMarked( false )
{
  m_pMimeType = KMimeType::mimeType( mimeType );
  init( false );
}

void KonqFileItem::init( bool _determineMimeTypeOnDemand )
{
  // determine mode and/or permissions if unknown
  if ( m_fileMode == (mode_t) -1 || m_permissions == (mode_t) -1 )
  {
    mode_t mode = 0;
    if ( m_url.isLocalFile() )
    {
      /* directories may not have a slash at the end if
       * we want to stat() them; it requires that we
       * change into it .. which may not be allowed
       * stat("/is/unaccessible")  -> rwx------
       * stat("/is/unaccessible/") -> EPERM            H.Z.
       * This is the reason for the -1
       */
      struct stat buf;
      lstat( m_url.path( -1 ), &buf );
      mode = buf.st_mode;

      if ( S_ISLNK( mode ) )
      {
        m_bLink = true;
        stat( m_url.path( -1 ), &buf );
        mode = buf.st_mode;
      }
    }
    if ( m_fileMode == (mode_t) -1 )
      m_fileMode = mode & S_IFMT; // extract file type
    if ( m_permissions == (mode_t) -1 )
      m_permissions = mode & 07777; // extract permissions
  }

  // determine the mimetype
  if (!m_pMimeType && !_determineMimeTypeOnDemand )
    m_pMimeType = KMimeType::findByURL( m_url, m_fileMode, m_bIsLocalURL );

  //  assert (m_pMimeType);
}

void KonqFileItem::refresh()
{
  m_fileMode = (mode_t)-1;
  m_permissions = (mode_t)-1;
  init( false );
}

void KonqFileItem::refreshMimeType()
{
  m_pMimeType = 0L;
  init( false ); // Will determine the mimetype
}

QPixmap KonqFileItem::pixmap( KIconLoader::Size _size, bool bImagePreviewAllowed ) const
{
  if ( !m_pMimeType )
  {
    if ( S_ISDIR( m_fileMode ) )
     return KGlobal::iconLoader()->loadIcon( "folder", _size );

    return KGlobal::iconLoader()->loadIcon( "mimetypes/unknown", _size );
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

QString KonqFileItem::linkDest() const
{
  // Extract it from the KIO::UDSEntry
  KIO::UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == KIO::UDS_LINK_DEST )
      return (*it).m_str;
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use readlink() [if local URL]
  if ( m_bIsLocalURL )
  {
    char buf[1000];
    int n = readlink( m_url.path( -1 ), buf, 1000 );
    if ( n != -1 )
    {
      buf[ n ] = 0;
      return QString( buf );
    }
  }
  return QString::null;
}

long KonqFileItem::size() const
{
  // Extract it from the KIO::UDSEntry
  KIO::UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == KIO::UDS_SIZE )
      return (*it).m_long;
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    return buf.st_size;
  }
  return 0L;
}

QString KonqFileItem::time( unsigned int which ) const
{
  // Extract it from the KIO::UDSEntry
  KIO::UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == which )
      return makeTimeString( (time_t)((*it).m_long) );
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    time_t t = (which == KIO::UDS_MODIFICATION_TIME) ? buf.st_mtime :
               (which == KIO::UDS_ACCESS_TIME) ? buf.st_atime :
               (which == KIO::UDS_CREATION_TIME) ? buf.st_ctime : (time_t) 0;
    return makeTimeString( t );
  }
  return QString::null;
}

QString KonqFileItem::mimetype() const
{
  KonqFileItem * that = const_cast<KonqFileItem *>(this);
  return that->determineMimeType()->name();
}

KMimeType::Ptr KonqFileItem::determineMimeType()
{
  if ( !m_pMimeType )
  {
    //kdDebug(1203) << "finding mimetype for " << m_url.url() << endl;
    m_pMimeType = KMimeType::findByURL( m_url, m_fileMode, m_bIsLocalURL );
  }

  return m_pMimeType;
}

QString KonqFileItem::mimeComment()
{
 KMimeType::Ptr mType = determineMimeType();
 QString comment = mType->comment( m_url, false );
  if (!comment.isEmpty())
    return comment;
  else
    return mType->name();
}

QString KonqFileItem::iconName()
{
  return determineMimeType()->icon(m_url, false);
}

void KonqFileItem::run()
{
  (void) new KRun( m_url, m_fileMode, m_bIsLocalURL );
}

QString KonqFileItem::makeTimeString( time_t _time )
{
  QDateTime dt;
  dt.setTime_t(_time);

  return KGlobal::locale()->formatDateTime(dt);
}
