/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers
                  2002-2003 Douglas Hanley <douglash@caltech.edu>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA  02111-1307, USA.
*/

#ifndef __konq_tabs_h__
#define __konq_tabs_h__

#include "konq_frame.h"

#include <ktabwidget.h>

class QPixmap;
class QPopupMenu;
class QToolButton;

class KonqView;
class KonqViewManager;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;
class KSeparator;
class KProgress;
class KAction;

class KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
  Q_OBJECT
  friend class KonqFrame; //for emitting ctrlTabPressed() only, aleXXX

public:
  KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
		KonqViewManager* viewManager, const char * name = 0);
  virtual ~KonqFrameTabs();

  virtual void listViews( ChildViewList *viewList );

  virtual void saveConfig( KConfig* config, const QString &prefix, bool saveURLs,
			   KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  virtual void printFrameInfo( const QString& spaces );

  QPtrList<KonqFrameBase>* childFrameList() { return m_pChildFrameList; }

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const QString &url, QWidget* sender );

  virtual QWidget* widget() { return this; }
  virtual QCString frameType() { return QCString("Tabs"); }

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
                             const QPoint & p, bool showIt=FALSE );

  void moveTabBackward(int index);
  void moveTabForward(int index);


public slots:
  void slotCurrentChanged( QWidget* newPage );

signals:
  void ctrlTabPressed();
  void removeTabPopup();

protected:
  void resizeEvent( QResizeEvent * );

  uint tabBarWidthForMaxChars( uint );
    void refreshSubPopupMenuTab();

  QPtrList<KonqFrameBase>* m_pChildFrameList;

private slots:
  void slotContextMenu( QWidget*, const QPoint& );
  void slotCloseRequest( QWidget* );
  void slotMovedTab( int, int );
  void slotMouseMiddleClick();
  void slotMouseMiddleClick( QWidget* );

  void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);
  void slotReceivedDropEvent( QDropEvent* );
  void slotInitiateDrag( QWidget * );
  void slotReceivedDropEvent( QWidget *, QDropEvent * );
    void slotSubPopupMenuTabActivated( int);
private:
  QString constrainedTitle( QString title, int maxLength );

  KonqViewManager* m_pViewManager;
  QPopupMenu* m_pPopupMenu;
    QPopupMenu * m_pSubPopupMenuTab;
  uint m_CurrentMaxLength, m_maxLength, m_minLength;
  QToolButton* m_rightWidget;
  bool m_permanentCloseButtons;
};

#endif
