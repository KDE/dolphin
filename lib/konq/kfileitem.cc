/*
 * This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 **/

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "kfileitem.h"

#include <kmimetypes.h>
#include <kpixmapcache.h>
#include <klocale.h>
#include <krun.h>

KFileItem::KFileItem( UDSEntry& _entry, KURL& _url ) :
  m_entry( _entry ), 
  m_url( _url ), 
  m_bIsLocalURL( _url.isLocalFile() ),
  m_bMarked( false )
{
  // extract the mode and the filename from the UDS Entry
  m_mode = 0;
  m_bLink = false;
  m_pMimeType = 0;
  m_strText = QString::null;
  UDSEntry::Iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ ) {
    if ( (*it).m_uds == UDS_FILE_TYPE )
      m_mode = (mode_t)((*it).m_long);
    else if ( (*it).m_uds == UDS_NAME )
      m_strText = decodeFileName( (*it).m_str );
    else if ( (*it).m_uds == UDS_URL )
      m_url = KURL((*it).m_str);
    else if ( (*it).m_uds == UDS_MIME_TYPE )
      m_pMimeType = KMimeType::find((*it).m_str);
    else if ( (*it).m_uds == UDS_LINK_DEST )
      m_bLink = !(*it).m_str.isEmpty(); // we don't store the link dest
  }
  KFileItem::init(); // don't call derived methods !
}

KFileItem::KFileItem( QString _text, mode_t _mode, KURL& _url ) :
  m_entry(), // warning !
  m_url( _url ), 
  m_bIsLocalURL( _url.isLocalFile() ),
  m_strText( _text ),
  m_mode( _mode ),
  m_bLink( false /* TODO : pass as argument */ ),
  m_bMarked( false )
{
  KFileItem::init(); // don't call derived methods !
  // determine mode if unknown
  if ( m_mode == (mode_t) -1 )
  {
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
      stat( m_url.path( -1 ), &buf );
      m_mode = buf.st_mode;
    }
    else
      m_mode = 0;
  }

}

void KFileItem::init()
{
  assert(!m_strText.isNull());

  // determine the mimetype
  if (!m_pMimeType)
    m_pMimeType = KMimeType::findByURL( m_url, m_mode, m_bIsLocalURL );
  assert (m_pMimeType);
}

QPixmap* KFileItem::getPixmap( bool _mini ) const
{
  QPixmap * p = KPixmapCache::pixmapForMimeType( m_pMimeType, m_url, m_bIsLocalURL, _mini );
  if (!p)
    warning("Pixmap not found for mimetype %s",m_pMimeType->mimeType().ascii());
  return p;
}

bool KFileItem::acceptsDrops( QStringList& /* _formats */ ) const
{
  if ( strcmp( "inode/directory", m_pMimeType->mimeType() ) == 0 )
    return true;

  if ( !m_bIsLocalURL )
    return false;

  if ( strcmp( "application/x-desktop", m_pMimeType->mimeType() ) == 0 )
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
  // Extract from the UDSEntry the additionnal info we didn't get previously
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
  else if ( S_ISREG( m_mode ) )
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
  else if ( S_ISDIR ( m_mode ) )
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
  QString linkDest = QString::null;
  // Extract it from the UDSEntry
  UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == UDS_LINK_DEST )
      linkDest = (*it).m_str;
  return linkDest;
}

long KFileItem::size() const
{
  long size = 0L;
  // Extract it from the UDSEntry
  UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == UDS_SIZE )
      size = (*it).m_long;
  return size;
}

QString KFileItem::time( int which ) const
{
  // Extract it from the UDSEntry
  UDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == UDS_MODIFICATION_TIME )
      return makeTimeString( (*it) );
  return QString::null;
}

QString KFileItem::mimetype() const
{
  return m_pMimeType->name();
}

void KFileItem::run()
{
  (void) new KRun( m_url.url(), m_mode, m_bIsLocalURL );
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

const char* KFileItem::makeTimeString( const UDSAtom &_atom )
{
  static char buffer[ 100 ];
 
  time_t t = (time_t)_atom.m_long;
  struct tm* t2 = localtime( &t );
 
  strftime( buffer, 100, "%c", t2 );
 
  return buffer;
} 
