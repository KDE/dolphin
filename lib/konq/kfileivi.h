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

#ifndef __kfileivi_h__
#define __kfileivi_h__ $Id$

#include <qiconview.h>

class KFileItem;
class QPainter;

/**
 * KFileIVI (short form of "K - File - IconViewItem")
 * is, as expected, an improved QIconViewItem, because
 * it represents a file.
 * All the information about the file is contained in the KFileItem
 * pointer.
 */
class KFileIVI : public QIconViewItem
{
  Q_OBJECT
public:
  /**
   * Create an icon, within a qlistview, representing a file
   * @param _parent the parent widget
   * @param _fileitem the file item created by KDirLister
   */
  KFileIVI( QIconView *_iconview, KFileItem* _fileitem );
  virtual ~KFileIVI() { }

  /**
   * Handler for return (or single/double click) on ONE icon.
   * Runs the file through KRun.
   */
  virtual void returnPressed();
  
  /** @return the file item held by this instance */
  KFileItem * item() { return m_fileitem; }

  virtual bool acceptDrop( const QMimeSource *mime ) const;

  virtual void setKey( const QString &key );

signals:
  void dropMe( KFileIVI *item, QDropEvent *e );

protected:
  virtual void dropped( QDropEvent *e );
  virtual void paintItem( QPainter *p );

  /** Pointer to the file item in KDirLister's list */
  KFileItem* m_fileitem;
};

#endif
