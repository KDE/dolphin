/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#include <unistd.h>

#include "konq_kfmview.h"
#include "kfmviewprops.h"

#include <kmimetypes.h>
#include <kpixmapcache.h>
#include <klocale.h>

KonqKfmViewItem::KonqKfmViewItem( UDSEntry& _entry, KURL& _url ) :
  m_entry( _entry ), 
  m_url( _url ), 
  m_bMarked( false ),
  m_bIsLocalURL( _url.isLocalFile() )
{
  init();
}

void KonqKfmViewItem::init()
{
  mode_t mode = 0;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

  m_pMimeType = KMimeType::findByURL( m_url, mode, m_bIsLocalURL );

}

bool KonqKfmViewItem::acceptsDrops( QStrList& /* _formats */ )
{
  if ( strcmp( "inode/directory", m_pMimeType->mimeType() ) == 0 )
    return true;

  if ( !m_bIsLocalURL )
    return false;

  if ( strcmp( "application/x-kdelnk", m_pMimeType->mimeType() ) == 0 )
    return true;

  // Executable, shell script ... ?
  if ( access( m_url.path(), X_OK ) == 0 )
    return true;

  return false;
}

void KonqKfmViewItem::returnPressed()
{
  //TODO
}

QString KonqKfmViewItem::getStatusBarInfo()
{
  QString comment = m_pMimeType->comment( m_url, false );
  QString text;
  QString linkDest;

  long size   = 0;
  mode_t mode = 0;

  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
  {
    if ( it->m_uds == UDS_SIZE )
      size = it->m_long;
    else if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;
    else if ( it->m_uds == UDS_LINK_DEST )
      linkDest = it->m_str.c_str();
    else if ( it->m_uds == UDS_NAME )
      text = it->m_str.c_str();
  }

  QString text2 = text.copy();

  if ( m_url.isLocalFile() )
  {
    if ( mode & S_ISVTX /* S_ISLNK( mode ) */ )
    {
      QString tmp;
      if ( comment.isEmpty() )
	tmp = i18n ( "Symbolic Link" );
      else
	tmp.sprintf(i18n( "%s (Link)" ), comment.data() );
      text += "->";
      text += linkDest;
      text += "  ";
      text += tmp.data();
    }
    else if ( S_ISREG( mode ) )
    {
      text += " ";
      if (size < 1024)
	text.sprintf( "%s (%ld %s)",
		      text2.data(), (long) size,
		      i18n( "bytes" ).ascii());
      else
      {
	float d = (float) size/1024.0;
	text.sprintf( "%s (%.2f K)", text2.data(), d);
      }
      text += "  ";
      text += comment.data();
    }
    else if ( S_ISDIR( mode ) )
    {
      text += "/  ";
      text += comment.data();
    }
    else
    {
      text += "  ";
      text += comment.data();
    }	
    return text;
  }
  else
    return m_url.decodedURL();
}

