/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (c) 1999 David Faure <fauren@kde.org>

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

#ifndef __konq_bgnd_h
#define __konq_bgnd_h

#include <qgroupbox.h>
#include <qstring.h>
#include <qpixmap.h>

#include <kdialogbase.h>

class QComboBox;
class QPushButton;

/**
 * Reuseable widget that is the core of the background-image dialog.
 * It features a combobox with a list of available 'wallpaper' pixmaps,
 * and an area to show the image, auto-sizing.
 */
class KBgndDialogPage : public QGroupBox
{
  Q_OBJECT
public:
  /**
   * @param parent
   * @param pixmapFile
   * @param instance
   * @param resource the resource to use to list the available pixmaps. e.g. "wallpapers"
   */
  KBgndDialogPage( QWidget * parent, const QString & pixmapFile, KInstance *instance, const char * resource );
  virtual ~KBgndDialogPage();

  QPixmap pixmap() { return m_wallPixmap; }
  QString pixmapFile() { return m_wallFile; }

public slots:
  void slotWallPaperChanged( int );
  void slotBrowse();

protected:
  void showSettings( QString fileName );
  void loadWallPaper();
  virtual void resizeEvent ( QResizeEvent * );

  QPushButton * m_browseButton;
  QComboBox * m_wallBox;
  QFrame * m_wallWidget;
  QPixmap m_wallPixmap;
  QString m_wallFile;
  int imageX, imageW, imageH, imageY;
  KInstance *m_instance;
  QCString m_resource;
};


/**
 * Dialog for configuring the background image
 * Currently it defines and shows the pixmaps under the tiles resource
 */
class KonqBgndDialog : public KDialogBase
{
  Q_OBJECT
public:
  /**
   * Constructor
   */
  KonqBgndDialog( const QString & pixmapFile, KInstance *instance );
  ~KonqBgndDialog();

  QPixmap pixmap() { return m_propsPage->pixmap(); }
  QString pixmapFile() { return m_propsPage->pixmapFile(); }

private:
  KBgndDialogPage * m_propsPage;
};

#endif

