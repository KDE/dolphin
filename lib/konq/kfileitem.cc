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

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "kfileitem.h"

#include <kglobal.h>
#include <kmimetype.h>
#include <qpixmap.h>
#include <klocale.h>
#include <krun.h>

KFileItem::KFileItem( const KUDSEntry& _entry, KURL& _url ) :
  m_entry( _entry ), 
  m_url( _url ), 
  m_bIsLocalURL( _url.isLocalFile() ),
  m_fileMode( (mode_t)-1 ),
  m_permissions( (mode_t)-1 ),
  m_bLink( false ),
  m_pMimeType( 0 ),
  m_bMarked( false )
{
  // extract the mode and the filename from the UDS Entry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ ) {
    if ( (*it).m_uds == UDS_FILE_TYPE )
      m_fileMode = (mode_t)((*it).m_long);
    else if ( (*it).m_uds == UDS_ACCESS)
      m_permissions = (mode_t)((*it).m_long);
    else if ( (*it).m_uds == UDS_USER)
      m_user = ((*it).m_str);
    else if ( (*it).m_uds == UDS_GROUP)
      m_group = ((*it).m_str);
    else if ( (*it).m_uds == UDS_NAME )
      m_strText = decodeFileName( (*it).m_str );
    else if ( (*it).m_uds == UDS_URL )
      m_url = KURL((*it).m_str);
    else if ( (*it).m_uds == UDS_MIME_TYPE )
      m_pMimeType = KMimeType::mimeType((*it).m_str);
    else if ( (*it).m_uds == UDS_LINK_DEST )
      m_bLink = !(*it).m_str.isEmpty(); // we don't store the link dest
  }
  init();
}

KFileItem::KFileItem( QString _text, mode_t _mode, const KURL& _url ) :
  m_entry(), // warning !
  m_url( _url ), 
  m_bIsLocalURL( _url.isLocalFile() ),
  m_strText( _text ),
  m_fileMode ( _mode ), // temporary
  m_permissions( (mode_t) -1 ),
  m_bLink( false ),
  m_bMarked( false )
{
  init();
}

void KFileItem::init()
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
      m_permissions = mode & 0x1FF; // extract permissions 
  }

  assert(!m_strText.isNull());

  // determine the mimetype
  if (!m_pMimeType)
    m_pMimeType = KMimeType::findByURL( m_url, m_fileMode, m_bIsLocalURL );
  assert (m_pMimeType);
}

QPixmap KFileItem::pixmap( KIconLoader::Size _size ) const
{
  QPixmap p = m_pMimeType->pixmap( m_url, _size );
  if (p.isNull())
    warning("Pixmap not found for mimetype %s",m_pMimeType->name().latin1());
  return p;
}

bool KFileItem::acceptsDrops() const
{
  // Any directory : yes
  if ( mimetype() == "inode/directory" )
    return true;

  // But only local .desktop files and executables
  if ( !m_bIsLocalURL )
    return false;

  if ( mimetype() == "application/x-desktop")
    return true;
  
  // Executable, shell script ... ?
  if ( access( m_url.path(), X_OK ) == 0 )
    return true;
  
  return false;
}

QString KFileItem::getStatusBarInfo() const
{
  QString comment = m_pMimeType->comment( m_url, false );
  QString text = m_strText;
  // Extract from the UDSEntry the additional info we didn't get previously
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
        text = QString("%1 (%2 %3)").arg(text2).arg((long) mySize).arg(i18n("bytes"));
      else
      {
	float d = (float) mySize/1024.0;
        text = QString("%1 (%2 K)").arg(text2).arg(d, 0, 'f', 2); // was %.2f
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

QString KFileItem::linkDest() const
{
  // Extract it from the UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == UDS_LINK_DEST )
      return (*it).m_str;
  // If not in the UDSEntry, or if UDSEntry empty, use readlink() [if local URL]
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

long KFileItem::size() const
{
  // Extract it from the UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == UDS_SIZE )
      return (*it).m_long;
  // If not in the UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    return buf.st_size;
  }
  return 0L;
}

QString KFileItem::time( unsigned int which ) const
{
  // Extract it from the UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == which )
      return makeTimeString( (time_t)((*it).m_long) );
  // If not in the UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    time_t t = (which == UDS_MODIFICATION_TIME) ? buf.st_mtime :
               (which == UDS_ACCESS_TIME) ? buf.st_atime :
               (which == UDS_CREATION_TIME) ? buf.st_ctime : (time_t) 0;
    return makeTimeString( t );
  }
  return QString::null;
}

QString KFileItem::mimetype() const
{
  return m_pMimeType->name();
}

QString KFileItem::mimeComment() const
{
  if (!m_pMimeType->comment(m_url, false).isEmpty())
    return m_pMimeType->comment(m_url, false);
  else
    return m_pMimeType->name();
}

QString KFileItem::iconName() const
{
  return m_pMimeType->icon(m_url, false);
}

void KFileItem::run()
{
  (void) new KRun( m_url.url(), m_fileMode, m_bIsLocalURL );
}

QString KFileItem::encodeFileName( const QString & _str )
{
  QString str( _str );

  int i = 0;
  while ( ( i = str.find( "%", i ) ) != -1 )
  {
    str.replace( i, 1, "%%");
    i += 2;
  }
  while ( ( i = str.find( "/" ) ) != -1 )
      str.replace( i, 1, "%2f");
  return str;
}

QString KFileItem::decodeFileName( const QString & _str )
{
  QString str( _str );

  int i = 0;
  while ( ( i = str.find( "%%", i ) ) != -1 )
  {
    str.replace( i, 2, "%");
    i++;
  }

  while ( ( i = str.find( "%2f" ) ) != -1 )
      str.replace( i, 3, "/");
  while ( ( i = str.find( "%2F" ) ) != -1 )
      str.replace( i, 3, "/");
  return str;
}

QString KFileItem::makeTimeString( time_t _time )
{
  QDateTime dt;
  dt.setTime_t(_time);

  return KGlobal::locale()->formatDateTime(dt);
} 
