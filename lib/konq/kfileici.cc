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

#include "kfileici.h"
#include "kfileitem.h"

KFileICI::KFileICI( KIconContainer *_container, KFileItem* _fileitem )
  : KIconContainerItem( _container ), // parent constructor
    m_fileitem( _fileitem )
{
  // Set the item text (the one displayed) from the text computed by KFileItem
  setText( m_fileitem->getText() );
  // Set the item name from the url hold by KFileItem
  setName( m_fileitem->url().url() );
  // Determine the item pixmap from one determined by KFileItem
  KIconContainer::DisplayMode mode = _container->displayMode();
  QPixmap *p = m_fileitem->getPixmap( mode == KIconContainer::Vertical ||
                                      mode == KIconContainer::SmallVertical );
  if (p) setPixmap( *p );
}

void KFileICI::refresh( bool _display_mode_changed )
{
  if ( _display_mode_changed ) 
  {
    KIconContainer::DisplayMode mode = m_pContainer->displayMode();
    QPixmap *p = m_fileitem->getPixmap( mode == KIconContainer::Vertical ||
                                        mode == KIconContainer::SmallVertical );
    if (p) setPixmap( *p ); // store it in the item (KIconContainerItem)
  }
  
  KIconContainerItem::refresh( _display_mode_changed );
}
bool KFileICI::acceptsDrops( QStringList &_formats )
{
  if ( m_fileitem->mimetype() == "inode/directory" )
    return true;
  else
    return KIconContainerItem::acceptsDrops( _formats );
}

void KFileICI::paint( QPainter* _painter, bool _drag )
{
  if ( m_fileitem->isLink() )
  {
    QFont f = _painter->font();
    f.setItalic( true );
    _painter->setFont( f );
  }

  //debug("calling KICI::paint");
  
  KIconContainerItem::paint( _painter, _drag );
}

void KFileICI::returnPressed()
{
  m_fileitem->run();
}

bool KFileICI::isSmallerThen( const KIconContainerItem &_item )
{
  // Special handling for fixed pos items
  if ( hasFixedPos() && !_item.hasFixedPos() )
    return true;

  if ( m_pContainer->sortCriterion() != KIconContainerItem::Size )
    return KIconContainerItem::isSmallerThen( _item );

  bool res = ( m_fileitem->size() < ((KFileICI&)_item).item()->size() );
    
  if ( m_pContainer->sortDirection() == KIconContainerItem::Descending )
    res = !res;
    
  return res;
}
