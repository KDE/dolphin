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

#ifndef KONQ_FRAMECONTAINER_H
#define KONQ_FRAMECONTAINER_H

#include "konq_frame.h"
#include <QSplitter>

class KonqFrameContainerBase : public KonqFrameBase
{
public:
  virtual ~KonqFrameContainerBase() {}

  /**
   * Call this after inserting a new frame into the splitter.
   */
  virtual void insertChildFrame( KonqFrameBase * frame, int index = -1 ) = 0;
  /**
   * Call this before deleting one of our children.
   */
  virtual void removeChildFrame( KonqFrameBase * frame ) = 0;

  //inherited
  virtual void printFrameInfo( const QString& spaces );

  virtual QByteArray frameType() { return QByteArray("ContainerBase"); }

  virtual void reparentFrame(QWidget * parent,
                             const QPoint & p ) = 0;

  virtual KonqFrameBase* activeChild() { return m_pActiveChild; }

  virtual void setActiveChild( KonqFrameBase* activeChild ) { m_pActiveChild = activeChild;
                                                              m_pParentContainer->setActiveChild( this ); }

  virtual void activateChild() { if (m_pActiveChild) m_pActiveChild->activateChild(); }

  virtual KonqView* activeChildView() { if (m_pActiveChild) return m_pActiveChild->activeChildView();
                                        else return 0L; }

protected:
  KonqFrameContainerBase() {}

  KonqFrameBase* m_pActiveChild;
};

/**
 * With KonqFrameContainers and @refKonqFrames we can create a flexible
 * storage structure for the views. The top most element is a
 * KonqFrameContainer. It's a direct child of the MainView. We can then
 * build up a binary tree of containers. KonqFrameContainers are the nodes.
 * That means that they always have two children. Which are either again
 * KonqFrameContainers or, as leaves, KonqFrames.
 */

class KonqFrameContainer : public QSplitter, public KonqFrameContainerBase
{
  Q_OBJECT
  friend class KonqFrame; //for emitting ctrlTabPressed() only, aleXXX
public:
  KonqFrameContainer( Qt::Orientation o,
                      QWidget* parent,
                      KonqFrameContainerBase* parentContainer );
  virtual ~KonqFrameContainer();

  virtual void listViews( ChildViewList *viewList );

  virtual void saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  KonqFrameBase* firstChild() { return m_pFirstChild; }
  KonqFrameBase* secondChild() { return m_pSecondChild; }
  KonqFrameBase* otherChild( KonqFrameBase* child );

  virtual void printFrameInfo( const QString& spaces );

  void swapChildren();

  virtual void setTitle( const QString &title, QWidget* sender );
  virtual void setTabIcon( const KUrl &url, QWidget* sender );

  virtual QWidget* asQWidget() { return this; }
  virtual QByteArray frameType() { return QByteArray("Container"); }

  /**
   * Call this after inserting a new frame into the splitter.
   */
  void insertChildFrame( KonqFrameBase * frame, int index = -1 );
  /**
   * Call this before deleting one of our children.
   */
  void removeChildFrame( KonqFrameBase * frame );

  //inherited
  virtual void reparentFrame(QWidget * parent,
                             const QPoint & p );

  //make this one public
  int idAfter( QWidget* w ){ return QSplitter::indexOf( w ) + 1; }

  void setAboutToBeDeleted() { m_bAboutToBeDeleted = true; }

  //inherited
  virtual void childEvent( QChildEvent * );

Q_SIGNALS:
  void ctrlTabPressed();
  void setRubberbandCalled();

protected:
  KonqFrameBase* m_pFirstChild;
  KonqFrameBase* m_pSecondChild;
  bool m_bAboutToBeDeleted;
};

#endif /* KONQ_FRAMECONTAINER_H */

