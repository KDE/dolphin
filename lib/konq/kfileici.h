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

#include "kiconcontainer.h"

class KFileItem;
class QPainter;

/**
 * KFileICI (short form of "K - File - IconContainerItem")
 * is, as expected, an improved iconcontainer item, because
 * it represents a file.
 * All the information about the file is contained in the KFileItem
 * pointer.
 */
class KFileICI : public KIconContainerItem
{
public:
  /**
   * Create an icon, within an iconcontainer, representing a file
   * @param _parent the parent widget, an icon container
   * @param _fileitem the file item created by KDirLister
   */
  KFileICI( KIconContainer *_container, KFileItem* _fileitem );
  virtual ~KFileICI() { }

  /**
   * Handler for return (or single/double click) on ONE icon.
   * Runs the file through KRun.
   */
  virtual void returnPressed();
  
  /** @return the file item held by this instance */
  KFileItem * item() { return m_fileitem; }

  virtual bool acceptsDrops( QStringList& _formats );

protected:
  virtual void paint( QPainter* _painter, bool _drag );
  virtual void refresh( bool _display_mode_changed );
  
  virtual bool isSmallerThen( const KIconContainerItem &_item );
  
  /** Pointer to the file item in KDirLister's list */
  KFileItem* m_fileitem;
};

