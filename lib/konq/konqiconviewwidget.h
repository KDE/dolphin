/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __konq_iconviewwidget_h__
#define __konq_iconviewwidget_h__

#include <kiconloader.h>
#include <qiconview.h>
#include <kurl.h>
#include <kfileitem.h>

class KonqSettings;
class KFileIVI;

/**
 * A file-aware icon view, implementing drag'n'drop, KDE icon sizes, 
 * user settings, ...
 * Used by kdesktop and konq_iconview
 */
class KonqIconViewWidget : public QIconView
{
  Q_OBJECT
public:
  KonqIconViewWidget( QWidget *parent = 0L, const char *name = 0L, WFlags f = 0 );
  virtual ~KonqIconViewWidget() {}

  void initConfig();

  void setSize( KIconLoader::Size size );
  KIconLoader::Size size() { return m_size; }

  void setURL ( const QString & kurl ) { m_url = kurl; }

  /** Made public for konq_iconview (copy) */
  virtual QDragObject *dragObject();

  /**
   * Get list of selected KFileItems
   */
  KFileItemList selectedFileItems();

protected slots:

  virtual void slotDrop( QDropEvent *e );
  /** connect each item to this */
  virtual void slotDropItem( KFileIVI *item, QDropEvent *e );

protected:
  /** Common to slotDrop and slotDropItem */
  virtual void dropStuff( KFileIVI *item, QDropEvent *ev );

  virtual void drawBackground( QPainter *p, const QRect &r );

  void initDragEnter( QDropEvent *e );

  QString m_url;

  KIconLoader::Size m_size;

  /** Konqueror settings */
  KonqSettings * m_pSettings;
};

#endif
