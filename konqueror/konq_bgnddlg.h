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

#include <qstring.h>
#include <qpixmap.h>

#include <kurl.h>
#include <kdialogbase.h>

class KFileItem;
class QComboBox;
class QPushButton;

class DirPropsPage : public QWidget
{
  Q_OBJECT
public:
  DirPropsPage( QWidget * parent, const KURL & dirURL );
  virtual ~DirPropsPage();

  QPixmap pixmap() { return m_wallPixmap; }

public slots:
  void slotWallPaperChanged( int );
  void slotBrowse();
  void slotApply();
  void slotApplyGlobal();

protected:
  void showSettings( QString filename );
  void loadWallPaper();
  virtual void resizeEvent ( QResizeEvent * );

  const KURL & m_url;
  QPushButton * m_browseButton;
  KFileItem * m_fileitem;
  QComboBox * m_wallBox;
  QWidget * m_wallWidget;
  QPixmap m_wallPixmap;
  QString m_wallFile;
  int imageX, imageW, imageH, imageY;
};


/**
 * Dialog for configuring the background image for a directory
 */
class KonqBgndDialog : public KDialogBase
{
  Q_OBJECT
public:
  /**
   * Constructor
   */
  KonqBgndDialog( const KURL & dirURL );
  ~KonqBgndDialog();

  QPixmap pixmap() { return m_propsPage->pixmap(); }

private:
  DirPropsPage * m_propsPage;
};

#endif

