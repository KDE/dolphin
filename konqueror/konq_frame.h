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

#include "konq_factory.h"

#include <qguardedptr.h>
#include <qcolor.h>
#include <qwidget.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qlabel.h>

#include <kpixmap.h>
#include <kpixmapeffect.h>

class BrowserView;
class QPixmap;
class QVBoxLayout;
class OPFrame;
class KonqChildView;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainer;
class KConfig;
class KSeparator;
class KProgress;

namespace KParts
{
  class ReadOnlyPart;
};

/** A CheckBox with a special paintEvent(). It looks like the
 unchecked radiobutton in b2k style if unchecked and contains a little
 anchor if checked.
 */
class KonqCheckBox : public QCheckBox
{
public:
    KonqCheckBox(QWidget *parent=0, const char *name=0)
        : QCheckBox(parent, name){}
protected:
    void paintEvent(QPaintEvent *ev);
};



/**
 * The KonqFrameStatusBar indicates wether a view is active or not.
 */
class KonqFrameStatusBar : public QWidget
{
  Q_OBJECT

   public:
      KonqFrameStatusBar( KonqFrame *_parent = 0L, const char *_name = 0L );
      ~KonqFrameStatusBar() {}
      QCheckBox *passiveModeCheckBox() const { return m_pPassiveModeCheckBox; }
      /**
       * Shows the passiveModeCheckBox and the little green led
       */
      void showStuff();
      /**
       * Hides the passiveModeCheckBox and the little green led
       */
      void hideStuff();

   public slots:
      void slotConnectToNewView(KParts::ReadOnlyPart *oldOne,KParts::ReadOnlyPart *newOne);
      void slotLoadingProgress( int percent );
      void slotSpeedProgress( int bytesPerSecond );

   signals:
      /**
       * This signal is emitted when the user clicked the bar.
       */
      void clicked();

      void passiveModeChange( bool mode );
   protected slots:
      void slotDisplayStatusText(const QString& text);

   protected:
      virtual bool eventFilter(QObject*,QEvent *);
      virtual void resizeEvent( QResizeEvent* );
      virtual void mousePressEvent( QMouseEvent* );
      /**
       * Brings up the context menu for this frame
       */
      virtual void splitFrameMenu();

      virtual void paintEvent(QPaintEvent *e);
      KonqFrame* m_pParentKonqFrame;
      QCheckBox *m_pPassiveModeCheckBox;
      KProgress *m_progressBar;
      QLabel statusLabel;
      int m_yOffset;
      bool m_showLed;
};

typedef QList<KonqChildView> ChildViewList;

class KonqFrameBase
{
 public:
  virtual void saveConfig( KConfig* config, const QString &prefix, int id = 0, int depth = 0 ) = 0;

  virtual void reparent( QWidget* parent, Qt::WFlags f,
			 const QPoint & p, bool showIt=FALSE ) = 0;

  virtual KonqFrameContainer* parentContainer() = 0;
  virtual QWidget* widget() = 0;

  virtual void listViews( ChildViewList *viewList ) = 0;
  virtual QString frameType() = 0;

 protected:
  KonqFrameBase() {}
  virtual ~KonqFrameBase() {}
};

/**
 * The KonqFrame is the actual container for the views. It takes care of the
 * widget handling i.e. it attaches/detaches the view widget and activates
 * them on click at the statusbar.
 *
 * KonqFrame makes the difference between built-in views and remote ones.
 * We create a layout in it (with the KonqFrameStatusBar as top item in the layout)
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
  ~KonqFrame();

  /**
   * Attach a view to the KonqFrame.
   * @param view the view to attach (instead of the current one, if any)
   */
  KParts::ReadOnlyPart *attach( const KonqViewFactory &viewFactory );

  void attachInternal();

  /**
   * Returns the view that is currently connected to the Frame.
   */
  KParts::ReadOnlyPart *view( void );

  bool isActivePart();

  KonqChildView* childView() { return m_pChildView; }
  void setChildView( KonqChildView* child );
  void listViews( ChildViewList *viewList );

  void saveConfig( KConfig* config, const QString &prefix, int id = 0, int depth = 0 );

  void reparent(QWidget * parent, WFlags f,
		const QPoint & p, bool showIt=FALSE );

  KonqFrameContainer* parentContainer();
  QWidget* widget() { return this; }
  virtual QString frameType() { return QString("View"); }

  QVBoxLayout *layout() { return m_pLayout; }

  KonqFrameStatusBar *statusbar() const { return m_pStatusBar; }

  QFrame *metaViewFrame() const { return m_metaViewFrame; }

  void detachMetaView();

  void attachMetaView( KParts::ReadOnlyPart *view, bool enableMetaViewFrame, const QMap<QString,QVariant> &framePropertyMap );

public slots:

  /**
   * Is called when the frame statusbar has been clicked
   */
  void slotStatusBarClicked();

  void slotPassiveModeChange( bool mode );

protected:
  virtual void paintEvent( QPaintEvent* );

  QVBoxLayout *m_metaViewLayout;
  QFrame *m_metaViewFrame;
  QVBoxLayout *m_pLayout;
  KonqChildView *m_pChildView;

  QGuardedPtr<KParts::ReadOnlyPart> m_pView;

  KSeparator *m_separator;
  KonqFrameStatusBar* m_pStatusBar;
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
		      QWidget* parent,
		      const char * name = 0);
  ~KonqFrameContainer() {}

  void listViews( ChildViewList *viewList );

  void saveConfig( KConfig* config, const QString &prefix, int id = 0, int depth = 0 );

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
