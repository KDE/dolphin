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

#include "kfileivi.h"
#include "kfileitem.h"

KFileIVI::KFileIVI( QIconView *_iconview, KFileItem* _fileitem )
  : QIconViewItem( _iconview, _fileitem->text(), 
  // HACK : As Reggie says, we need to build a full QIconSet
                   QIconSet( _fileitem->pixmap( KIconLoader::Medium ), QIconSet::Large ) ),
    m_fileitem( _fileitem )
{
  setDropEnabled( m_fileitem->mimetype() == "inode/directory" );
  if ( m_fileitem->isLink() )
  {
    QFont newFont = font();
    newFont.setItalic( true ); // FIXME
    setFont( newFont );
  }
}

bool KFileIVI::acceptDrop( const QMimeSource *mime ) const
{
  if ( mime->provides( "text/uri-list" ) && m_fileitem->acceptsDrops() )
    return true;
  else
    return QIconViewItem::acceptDrop( mime );
}

void KFileIVI::setKey( const QString &key )
{
  QString theKey = key;
  
  if ( m_fileitem->mimetype() == "inode/directory" )
    theKey.prepend( '0' );
  else
    theKey.prepend( '1' );

  QIconViewItem::setKey( theKey );
}

void KFileIVI::dropped( QDropEvent *e )
{
  emit dropMe( this, e );
}

void KFileIVI::returnPressed()
{
  m_fileitem->run();
}

#include "kfileivi.moc"
