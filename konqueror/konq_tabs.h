/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers <konq-e@kde.org>
    Copyright (C) 2002-2003 Douglas Hanley <douglash@caltech.edu>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#ifndef __konq_tabs_h__
#define __konq_tabs_h__

#include "konq_framecontainer.h"

#include <ktabwidget.h>
//Added by qt3to4:
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QList>

class QPixmap;
class QMenu;
class QToolButton;

class KonqView;
class KonqViewManager;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;
class KSeparator;
class KAction;

class KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
  Q_OBJECT
  friend class KonqFrame; //for emitting ctrlTabPressed() only, aleXXX

public:
  KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
		KonqViewManager* viewManager);
  virtual ~KonqFrameTabs();

  virtual void listViews( ChildViewList *viewList );

  virtual void saveConfig( KConfig* config, const QString &prefix, bool saveURLs,
			   KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  virtual void printFrameInfo( const QString& spaces );

  QList<KonqFrameBase*>* childFrameList() { return m_pChildFrameList; }

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const KUrl &url, QWidget* sender );

  virtual QWidget* asQWidget() { return this; }
  virtual QByteArray frameType() { return QByteArray("Tabs"); }

  void activateChild();

  /**
   * Call this after inserting a new frame into the splitter.
   */
  void insertChildFrame( KonqFrameBase * frame, int index = -1);

  /**
   * Call this before deleting one of our children.
   */
  void removeChildFrame( KonqFrameBase * frame );

  //inherited
  virtual void reparentFrame(QWidget * parent,
                             const QPoint & pos );

  void moveTabBackward(int index);
  void moveTabForward(int index);


public Q_SLOTS:
  void slotCurrentChanged( QWidget* newPage );
  void setAlwaysTabbedMode( bool );

Q_SIGNALS:
  void ctrlTabPressed();
  void removeTabPopup();

protected:
  void refreshSubPopupMenuTab();
  void hideTabBar();

  QList<KonqFrameBase*>* m_pChildFrameList;

private Q_SLOTS:
  void slotContextMenu( const QPoint& );
  void slotContextMenu( QWidget*, const QPoint& );
  void slotCloseRequest( QWidget* );
  void slotMovedTab( int, int );
  void slotMouseMiddleClick();
  void slotMouseMiddleClick( QWidget* );

  void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);
  void slotReceivedDropEvent( QDropEvent* );
  void slotInitiateDrag( QWidget * );
  void slotReceivedDropEvent( QWidget *, QDropEvent * );
  void slotSubPopupMenuTabActivated( int );

private:
  KonqViewManager* m_pViewManager;
  QMenu* m_pPopupMenu;
  QMenu* m_pSubPopupMenuTab;
  QToolButton* m_rightWidget;
  QToolButton* m_leftWidget;
  bool m_permanentCloseButtons;
  bool m_alwaysTabBar;
  bool m_MouseMiddleClickClosesTab;
  int m_closeOtherTabsId;
};

#endif
