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

#include <kdialogbase.h>

class KColorButton;
class KURLRequester;
class QButtonGroup;
class QRadioButton;

/**
 * Dialog for configuring the background
 * Currently it defines and shows the pixmaps under the tiles resource
 */
class KonqBgndDialog : public KDialogBase
{
  Q_OBJECT
public:
  /**
   * Constructor
   */
  KonqBgndDialog( QWidget* parent, const QString& pixmapFile,
                  const QColor& theColor, const QColor& defaultColor );
  ~KonqBgndDialog();

  QColor color() const;
  const QString& pixmapFile() const { return m_pixmapFile; }

private slots:
  void slotBackgroundModeChanged();
  void slotPictureChanged();
  void slotColorChanged();
  
private:
  void initPictures();
  void loadPicture( const QString& fileName );

  QColor m_color;
  QPixmap m_pixmap;
  QString m_pixmapFile;
  QFrame* m_preview;
  
  QButtonGroup* m_buttonGroup;
  QRadioButton* m_radioColor;
  QRadioButton* m_radioPicture;
  KURLRequester* m_comboPicture;
  KColorButton* m_buttonColor;
  
};

#endif
