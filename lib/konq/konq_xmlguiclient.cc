/* This file is part of the KDE project
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>
   Copyright (c) 1998, 1999 David Faure <faure@kde.org>

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

#include "konq_xmlguiclient.h"


KonqXMLGUIClient::KonqXMLGUIClient( ) : KXMLGUIClient( )
{
  attrName = QString::fromLatin1( "name" );
  prepareXMLGUIStuff( );
}
KonqXMLGUIClient::KonqXMLGUIClient( KXMLGUIClient *parent ) : KXMLGUIClient(parent )
{
  attrName = QString::fromLatin1( "name" );
  prepareXMLGUIStuff( );
}

void KonqXMLGUIClient::prepareXMLGUIStuff()
{
  m_doc = QDomDocument( "kpartgui" );

  QDomElement root = m_doc.createElement( "kpartgui" );
  m_doc.appendChild( root );
  root.setAttribute( attrName, "popupmenu" );

  m_menuElement = m_doc.createElement( "Menu" );
  root.appendChild( m_menuElement );
  m_menuElement.setAttribute( attrName, "popupmenu" );

  /*m_builder = new KonqPopupMenuGUIBuilder( this );
  m_factory = new KXMLGUIFactory( m_builder ); */
}

QDomElement KonqXMLGUIClient::DomElement( ) const {
  return m_menuElement;
}
QDomDocument KonqXMLGUIClient::domDocument( ) const {
  return m_doc;
}
void KonqXMLGUIClient::addAction( KAction *act, const QDomElement &menu )
{
  addAction( act->name(), menu );
}

void KonqXMLGUIClient::addAction( const char *name, const QDomElement &menu )
{
  static QString tagAction = QString::fromLatin1( "action" );

  QDomElement parent = menu;
  if ( parent.isNull() )
    parent = m_menuElement;

  QDomElement e = m_doc.createElement( tagAction );
  parent.appendChild( e );
  e.setAttribute( attrName, name );
}

void KonqXMLGUIClient::addSeparator( const QDomElement &menu )
{
  static QString tagSeparator = QString::fromLatin1( "separator" );

  QDomElement parent = menu;
  if ( parent.isNull() )
    parent = m_menuElement;

  parent.appendChild( m_doc.createElement( tagSeparator ) );
}

void KonqXMLGUIClient::addMerge( const QString &name )
{
  QDomElement merge = m_doc.createElement( "merge" );
  m_menuElement.appendChild( merge );
  if ( !name.isEmpty() )
    merge.setAttribute( attrName, name );
}

void KonqXMLGUIClient::addGroup( const QString &grp )
{
  QDomElement group = m_doc.createElement( "definegroup" );
  m_menuElement.appendChild( group );
  group.setAttribute( "name", grp );
}

KonqXMLGUIClient::~KonqXMLGUIClient( ){
}
