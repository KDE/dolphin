/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
 
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

#ifndef __konq_frame_h__
#define __konq_frame_h__

#include <qcolor.h>
#include <qwidget.h>
#include <kpixmap.h>
// #include <opFrame.h>
#include "openparts.h"

class QPixmap;

enum KonqFrameHeaderLook{
  PLAIN,
  H_SHADED,
  V_SHADED,
  PIXMAP
};

// class KonqFrame;

class KonqFrameHeader : public QWidget
{
  Q_OBJECT

public:
  KonqFrameHeader( OpenParts::Part_ptr view, QWidget *_parent = 0L, const char *_name = 0L );
  ~KonqFrameHeader() {}

  /**
   * Call this to change the part bound to the frame header.
   */
  void setPart( OpenParts::Part_ptr part ) { m_vPart = OpenParts::Part::_duplicate( part ); }

signals:
  void headerClicked();

protected: 
  virtual void paintEvent( QPaintEvent* );
  virtual void mousePressEvent( QMouseEvent* );

  void gradientFill(KPixmap &pm, QColor ca, QColor cb,bool vertShaded);

  //  KonqFrame* m_pParentKonqFrame;
  OpenParts::Part_ptr m_vPart;

private:
  KonqFrameHeaderLook frameHeaderLook;
  //int titleAnimation;
  //bool framedActiveTitle;
  //bool pixmapUnderTitleText;
  QPixmap* frameHeaderActive;
  QPixmap* frameHeaderInactive;
  KPixmap activeShadePm;
  KPixmap inactiveShadePm;
  QColor frameHeaderBlendActive;
  QColor frameHeaderBlendInactive;

};

/*
class KonqFrame : public OPFrame
{
  Q_OBJECT

public:
  KonqFrame( QWidget *_parent = 0L, const char *_name = 0L );
  ~KonqFrame() {}
 
  virtual bool attach( OpenParts::Part_ptr _part );

public slots:  
*/
  /**
   * Is called when the frame header has been clicked
   */
/*
  void slotHeaderClicked();

protected:
  virtual void resizeEvent( QResizeEvent* );
  virtual void paintEvent( QPaintEvent* );

  KonqFrameHeader* m_pHeader;
};
*/

#endif
