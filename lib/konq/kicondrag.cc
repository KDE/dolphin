/* This file is part of the KDE project
   Copyright (C) 1999 Torben Weis <weis@kde.org>
 
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

#include "kicondrag.h"

#include <stdlib.h>

KIconDrag::KIconDrag( const KIconDrag::IconList& _list, QWidget* _drag_source, const char* _name )
  : QDragObject( _drag_source, _name )
{
  m_lstIcons = _list;
}

KIconDrag::KIconDrag( QWidget * dragSource, const char * name )
    : QDragObject( dragSource, name )
{
}

KIconDrag::~KIconDrag()
{
}

void KIconDrag::setIcons( const KIconDrag::IconList &_list )
{
  m_lstIcons = _list;
}

static const char* icon_formats[] = {
	"url/url",
	"application/x-kiconlist",
	0 };

const char * KIconDrag::format(int i) const
{
  if ( i >= 0 && i < 3 )
    return icon_formats[i];
  return 0;
}

QByteArray KIconDrag::encodedData(const char* mime) const
{
  QByteArray a;
  if ( strcmp( mime, "url/url" ) == 0 )
  {
    int c = 0;
    IconList::ConstIterator it = m_lstIcons.begin();
    for ( ; it != m_lstIcons.end(); ++it )
    {
      int l = (*it).url.length();
      a.resize(c + l + 1 );
      memcpy( a.data() + c , (*it).url.ascii(), l );
      a[ c + l ] = 0;
      c += l + 1;
    }
    a.resize( c - 1 ); // chop off last nul
  }
  else if ( strcmp( mime, "application/x-kiconlist" ) == 0 )
  {
    int c = 0;
    IconList::ConstIterator it = m_lstIcons.begin();
    for ( ; it != m_lstIcons.end(); ++it )
    {
      QString d( "%1:%2:%3" );
      d = d.arg( (*it).pos.x() ).arg( (*it).pos.y() ).arg( (*it).url );
      int l = d.length();
      a.resize( c + l + 1 );
      memcpy( a.data() + c , d.ascii(), l );
      a[ c + l ] = 0;
      c += l + 1;
    }
    a.resize( c - 1 ); // chop off last nul
  }

  return a;
}

bool KIconDrag::canDecode( QMimeSource* e )
{
    for ( int i=0; icon_formats[i]; i++ )
	if ( e->provides( icon_formats[i] ) )
	    return TRUE;
    return FALSE;
}

bool KIconDrag::decode( QMimeSource* e, KIconDrag::IconList& _list )
{
  QByteArray payload = e->encodedData( "application/x-kiconlist" );
  if ( payload.size() )
  {
    _list.clear();
    uint c = 0;
    char* d = payload.data();
    while ( c < payload.size() )
    {
      uint f = c;
      while ( c < payload.size() && d[ c ] )
	c++;
      QString s;
      if ( c < payload.size() )
      {
	s = d + f ;
	c++;
      }
      else
      {
	QString tmp( QString(d + f).left( c - f + 1 ) );
	s = tmp;
      }
      Icon icon;
      icon.pos.setX( atoi( s.ascii() ) );
      int pos = s.find( ':' );
      if ( pos == -1 )
	return false;
      icon.pos.setY( atoi( s.ascii() + pos + 1 ) );
      pos = s.find( ':', pos + 1 );
      if ( pos == -1 )
	return false;
      icon.url = s.mid( pos + 1 );
      _list.append( icon );
    }
    return TRUE;
  }
  return FALSE;
}

bool KIconDrag::decode( QMimeSource* e, QStringList& _list )
{
  QByteArray payload = e->encodedData( "url/url" );
  if ( payload.size() )
  {
    _list.clear();
    uint c = 0;
    char* d = payload.data();
    while ( c < payload.size() )
    {
      uint f = c;
      while ( c < payload.size() && d[ c ] )
	c++;
      if ( c < payload.size() )
      {
	_list.append( d + f );
	c++;
      }
      else
      {
	QString s( QString(d + f).left(c - f + 1) );
	_list.append( s );
      }
    }
    return TRUE;
  }
  return FALSE;
}

#include "kicondrag.moc"
