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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __konq_frame_h__
#define __konq_frame_h__

#include "konq_factory.h"

#include <QPointer>
#include <QColor>
#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPixmap>
#include <QPaintEvent>
#include <QChildEvent>
#include <QList>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>

#include <kpixmapeffect.h>
#include <kstatusbar.h>

class QPixmap;
class QVBoxLayout;
class QHBoxLayout;
class QTimer;
class QPushButton;
class QToolButton;
class QProgressBar;

class KonqView;
class KonqViewManager;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KonqFrameTabs;
class KConfig;
class KSeparator;
class KAction;
class KSqueezedTextLabel;

namespace KParts
{
  class ReadOnlyPart;
}

/**
 * A CheckBox with a special paintEvent(). It looks like the
 * unchecked radiobutton in b2k style if unchecked and contains a little
 * anchor if checked.
 */
class KonqCheckBox : public QCheckBox
{
    Q_OBJECT // for classname
public:
    KonqCheckBox(QWidget *parent=0)
      :QCheckBox( parent ) {}
protected:
    void drawButton( QPainter * );
};



/**
 * The KonqFrameStatusBar is the statusbar under each konqueror view.
 * It indicates in particular whether a view is active or not.
 */
class KonqFrameStatusBar : public KStatusBar
{
  Q_OBJECT

   public:
      KonqFrameStatusBar( KonqFrame *_parent = 0 );
      virtual ~KonqFrameStatusBar();

      /**
       * Checks/unchecks the linked-view checkbox
       */
      void setLinkedView( bool b );
      /**
       * Shows/hides the active-view indicator
       */
      void showActiveViewIndicator( bool b );
      /**
       * Shows/hides the linked-view indicator
       */
      void showLinkedViewIndicator( bool b );
      /**
       * Updates the active-view indicator and the statusbar color.
       */
      void updateActiveStatus();

   public Q_SLOTS:
      void slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *oldOne,KParts::ReadOnlyPart *newOne);
      void slotLoadingProgress( int percent );
      void slotSpeedProgress( int bytesPerSecond );
      void slotDisplayStatusText(const QString& text);

      void slotClear();
      void message ( const QString & message );

   Q_SIGNALS:
      /**
       * This signal is emitted when the user clicked the bar.
       */
      void clicked();

      /**
       * The "linked view" checkbox was clicked
       */
      void linkedViewClicked( bool mode );

   protected:
      virtual bool eventFilter(QObject*,QEvent *);
      virtual void resizeEvent( QResizeEvent* );
      virtual void mousePressEvent( QMouseEvent* );
      /**
       * Brings up the context menu for this frame
       */
      virtual void splitFrameMenu();

      /**
       * Takes care of the statusbars size
       **/
      virtual void fontChange(const QFont &oldFont);

   private:
      KonqFrame* m_pParentKonqFrame;
      QCheckBox *m_pLinkedViewCheckBox;
      QProgressBar *m_progressBar;
      KSqueezedTextLabel *m_pStatusLabel;
      QLabel* m_led;
      QString m_savedMessage;
};


typedef QList<KonqView*> ChildViewList;

class KonqFrameBase
{
 public:
  virtual ~KonqFrameBase() {}

  virtual void saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id = 0, int depth = 0) = 0;

  virtual void copyHistory( KonqFrameBase *other ) = 0;

  virtual void printFrameInfo( const QString& spaces );

  virtual void reparentFrame( QWidget* parent,
                              const QPoint & p ) = 0;

  virtual KonqFrameContainerBase* parentContainer() const { return m_pParentContainer; }
  virtual void setParentContainer(KonqFrameContainerBase* parent) { m_pParentContainer = parent; }

  virtual void setTitle( const QString &title , QWidget* sender) = 0;
  virtual void setTabIcon( const KUrl &url, QWidget* sender ) = 0;

  virtual QWidget* widget() = 0;

  virtual void listViews( ChildViewList *viewList ) = 0;
  virtual QByteArray frameType() = 0;

  virtual void activateChild() = 0;

  virtual KonqView* activeChildView() = 0;

protected:
  KonqFrameBase() {}

  KonqFrameContainerBase* m_pParentContainer;
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
  KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer = 0L,
             const char *name = 0L );
  virtual ~KonqFrame();

  /**
   * Attach a view to the KonqFrame.
   * @param viewFactory the view to attach (instead of the current one, if any)
   */
  KParts::ReadOnlyPart *attach( const KonqViewFactory &viewFactory );

  /**
   * Filters the CTRL+Tab event from the views and emits ctrlTabPressed to
   make KonqMainWindow switch to the next view
   */
  virtual bool eventFilter(QObject*obj,QEvent *ev);

  /**
   * Inserts the part's widget and the statusbar into the layout
   */
  void attachInternal();

  /**
   * Inserts a widget at the top of the part's widget, in the layout
   * (used for the find functionality)
   */
  void insertTopWidget( QWidget * widget );

  /**
   * Returns the part that is currently connected to the Frame.
   */
  KParts::ReadOnlyPart *part() { return m_pPart; }
  /**
   * Returns the view that is currently connected to the Frame.
   */
  KonqView* childView() const;

  bool isActivePart();

  void setView( KonqView* child );
  virtual void listViews( ChildViewList *viewList );

  virtual void saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  virtual void printFrameInfo( const QString& spaces );

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const KUrl &url, QWidget* sender );

  virtual void reparentFrame(QWidget * parent,
                     const QPoint & p );

  //virtual KonqFrameContainerBase* parentContainer();
  virtual QWidget* widget() { return this; }
  virtual QByteArray frameType() { return QByteArray("View"); }

  QVBoxLayout *layout()const { return m_pLayout; }

  KonqFrameStatusBar *statusbar() const { return m_pStatusBar; }

  virtual void activateChild();

  virtual KonqView* activeChildView();

  QString title() const { return m_title; }

public Q_SLOTS:

  /**
   * Is called when the frame statusbar has been clicked
   */
  void slotStatusBarClicked();

  void slotLinkedViewClicked( bool mode );

  /**
   * Is called when 'Remove View' is called from the popup menu
   */
  void slotRemoveView();

protected:
  virtual void paintEvent( QPaintEvent* );

  QVBoxLayout *m_pLayout;
  QPointer<KonqView> m_pView;

  QPointer<KParts::ReadOnlyPart> m_pPart;

  KonqViewManager* m_pViewManager;

  KSeparator *m_separator;
  KonqFrameStatusBar* m_pStatusBar;

  QString m_title;
};

#endif
