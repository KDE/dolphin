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
#include <kpixmapeffect.h>
#include "openparts.h"

#include "konqueror.h"

class QPixmap;
class QVBoxLayout;
class OPFrame;
class QSplitter;
class KonqFrame;

enum KonqFrameHeaderLook{ Plain,  HORIZ, VERT, DIAG, CROSSDIAG, PYRAM,
			  RECT, PIPE, ELLIP, XPixmap };

/**
 * The KonqFrameHeader indicates wether a view is active or not. It uses the 
 * same colors a KWM does.
 */
class KonqFrameHeader : public QWidget
{
  Q_OBJECT

public:
  KonqFrameHeader( KonqFrame *_parent = 0L, const char *_name = 0L );
  ~KonqFrameHeader() {}

signals:
  /**
   * This signal is emmitted when the user clicked the header.
   */
  void headerClicked();

protected: 
  enum KPixmapEffect::GradientType mapShade( KonqFrameHeaderLook look);

  virtual void paintEvent( QPaintEvent* );
  virtual void mousePressEvent( QMouseEvent* );

  KonqFrame* m_pParentKonqFrame;

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

/**
 * The KonqFrame is the actual container for the views. It takes care of the 
 * widget handling i.e. it attaches/detaches the view widget and activates 
 * them on click at the header. 
 *
 * KonqFrame makes the difference between built-in views and remote ones.
 * We create a layout in it (with the FrameHeader as top item in the layout)
 * For builtin views we have the view as direct child widget of the layout
 * For remote views we have an OPFrame, having the view attached, as child 
 * widget of the layout
 */

class KonqFrame : public QWidget
{
  Q_OBJECT

public:
  KonqFrame( QSplitter *_parentSplitter = 0L, const char *_name = 0L );
  ~KonqFrame() {}
 
  /**
   * Attach a view to the KonqFrame.
   * @param view the view to attach (instead of the current one, if any)
   */
  void attach( Browser::View_ptr view );

  /** 
   * Detach attached view, before deleting myself, or attaching another one 
   */
  void detach( void );

  /**
   * Returns the view that is currently connected to the Frame.
   */
  Browser::View_ptr view( void );

  QSplitter* parentSplitter() { return m_pParentSplitter; }
  void setParentSplitter( QSplitter* newParent ) { m_pParentSplitter = newParent; }

  
public slots:  

  /**
   * Is called when the frame header has been clicked
   */
  void slotHeaderClicked();

protected:
  virtual void paintEvent( QPaintEvent* );

  OPFrame *m_pOPFrame;
  QVBoxLayout *m_pLayout;
  KonqFrameHeader* m_pHeader;
  Browser::View_ptr m_pView;
  QSplitter *m_pParentSplitter;
};

#endif
