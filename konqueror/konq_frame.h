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
#include <qsplitter.h>

#include <kpixmap.h>
#include <kpixmapeffect.h>

#include <browser.h>

class QPixmap;
class QVBoxLayout;
class OPFrame;
class KonqChildView;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainer;

enum KonqFrameHeaderLook{ Plain,  HORIZ, VERT, DIAG, CROSSDIAG, PYRAM,
			  RECT, PIPE, ELLIP, XPixmap };

/**
 * The KonqFrameHeader indicates wether a view is active or not. It uses the 
 * same colors and shading a KWM does.
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

typedef QList<KonqChildView> ChildViewList;

class KonqFrameBase
{
 public:
  virtual void saveConfig( KConfig* config, int id = 0, int depth = 0 ) = 0;

  virtual void reparent( QWidget* parent, Qt::WFlags f, 
			 const QPoint & p, bool showIt=FALSE ) = 0;

  virtual QWidget* widget() = 0;

  virtual void listViews( ChildViewList *viewList ) = 0;
  virtual QString frameType() = 0;

 protected:
  KonqFrameBase() {}
 ~KonqFrameBase() {}
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

class KonqFrame : public QWidget, public KonqFrameBase
{
  Q_OBJECT

public:
  KonqFrame( KonqFrameContainer *_parentContainer = 0L, 
	     const char *_name = 0L );
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

  KonqChildView* childView() { return m_pChildView; }
  void setChildView( KonqChildView* child ) { m_pChildView = child; }
  void listViews( ChildViewList *viewList );

  void saveConfig( KConfig* config, int id = 0, int depth = 0 );

  void reparent(QWidget * parent, WFlags f, 
		const QPoint & p, bool showIt=FALSE );
  
  KonqFrameContainer* parentContainer();
  QWidget* widget() { return this; }
  virtual QString frameType() { return QString("View"); }

public slots:  

  /**
   * Is called when the frame header has been clicked
   */
  void slotHeaderClicked();

protected:
  virtual void paintEvent( QPaintEvent* );

  OPFrame *m_pOPFrame;
  QVBoxLayout *m_pLayout;
  KonqChildView *m_pChildView;

  Browser::View_var m_vView;

  KonqFrameHeader* m_pHeader;
};

/**
 * With KonqFrameContainers and @refKonqFrames we can create a flexible 
 * storage structure for the views. The top most element is a 
 * KonqFrameContainer. It's a direct child of the MainView. We can then 
 * build up a binary tree of containers. KonqFrameContainers are the nodes. 
 * That means that they always have two childs. Which are either again 
 * KonqFrameContainers or, as leaves, KonqFrames.
 */

class KonqFrameContainer : public QSplitter, public KonqFrameBase
{
  Q_OBJECT

public:
  KonqFrameContainer( Orientation o, 
		      QWidget* parent=0, 
		      const char * name=0);
  ~KonqFrameContainer() {}

  void listViews( ChildViewList *viewList );

  void saveConfig( KConfig* config, int id = 0, int depth = 0 );

  KonqFrameBase* firstChild() { return m_pFirstChild; }
  void setFirstChild( KonqFrameBase* child ) { m_pFirstChild = child; }
  KonqFrameBase* secondChild() { return m_pSecondChild; }
  void setSecondChild( KonqFrameBase* child ) { m_pSecondChild = child; }
  KonqFrameBase* otherChild( KonqFrameBase* child );

  KonqFrameContainer* parentContainer();
  virtual QWidget* widget() { return this; }
  virtual QString frameType() { return QString("Container"); }

  //inherited
  void reparent(QWidget * parent, WFlags f, 
		const QPoint & p, bool showIt=FALSE );

  //make this one public
  int idAfter( QWidget* w ){ return QSplitter::idAfter( w ); }

protected:
  void childEvent( QChildEvent * ce );

  KonqFrameBase* m_pFirstChild;
  KonqFrameBase* m_pSecondChild;
};

#endif
