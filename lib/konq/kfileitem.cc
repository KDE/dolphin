/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <unistd.h>

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
  m_strText = QString::null;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
  {
    if ( it->m_uds == UDS_FILE_TYPE )
      m_mode = (mode_t)it->m_long;
    else if ( it->m_uds == UDS_NAME )
      m_strText = it->m_str;
  }
  KFileItem::init(); // don't call derived methods !
}

KFileItem::KFileItem( QString _text, mode_t _mode, KURL& _url ) :
  m_entry( 0 ), // warning !
  m_url( _url ), 
  m_bIsLocalURL( _url.isLocalFile() ),
  m_strText( _text ),
  m_mode( _mode ),
  m_bMarked( false )
{
  KFileItem::init(); // don't call derived methods !
}  

void KFileItem::init()
{
  assert(!m_strText.isNull());

  // determine the mimetype
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

bool KFileItem::isLink() const
{
  return S_ISLNK( m_mode );
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
  QString text;
  QString linkDest;

  long size   = 0;
  mode_t mode = 0;

  UDSEntry::const_iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
  {
    if ( it->m_uds == UDS_SIZE )
      size = it->m_long;
    else if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;
    else if ( it->m_uds == UDS_LINK_DEST )
      linkDest = it->m_str;
    else if ( it->m_uds == UDS_NAME )
      text = it->m_str;
  }

  QString text2 = text.copy();

  if ( m_url.isLocalFile() )
  {
    if ( S_ISLNK( mode ) )
    {
      QString tmp;
      if ( comment.isEmpty() )
	tmp = i18n ( "Symbolic Link" );
      else
        tmp = i18n("%1 (Link)").arg(comment);
      text += "->";
      text += linkDest;
      text += "  ";
      text += tmp;
    }
    else if ( S_ISREG( mode ) )
    {
      if (size < 1024)
        text = QString("%1 (%2 %3)").arg(text2).arg((long) size).arg(i18n("bytes"));
      else
      {
	float d = (float) size/1024.0;
        text = QString("%1 (%2 K)").arg(text2).arg(d, 0, 'f', 2); // was %.2f
      }
      text += "  ";
      text += comment;
    }
    else if ( S_ISDIR( mode ) )
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
  else
    return m_url.decodedURL();
}

void KFileItem::run()
{
  (void) new KRun( m_url.url(), m_mode, m_bIsLocalURL );
}
