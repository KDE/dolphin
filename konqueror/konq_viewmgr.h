/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
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

#ifndef __konq_viewmgr_h__
#define __konq_viewmgr_h__ $Id$

#include <qnamespace.h>
#include <qmap.h>

#include "konq_defs.h"

class QString;
class QStringList;
class KConfig;
class KonqMainView;
class KonqFrameContainer;
class KonqChildView;

namespace Browser
{
  class View;
  typedef View * View_ptr;
};

class KonqViewManager
{
public:
  KonqViewManager( KonqMainView *mainView );
  ~KonqViewManager();
  
  /**
   * Splits the view, depending on orientation, either horizontally or 
   * vertically. The first of the resulting views will contain the initial 
   * view, the other will be a new one with the same URL and the same view type
   */
  void splitView( Qt::Orientation orientation );

  /**
   * Does the same as the above, except that the second view will be the one 
   * provided by newView
   */
  void splitView( Qt::Orientation orientation,
		  Browser::View_ptr newView,
		  const QStringList &newViewServiceTypes );

  /**
   * Guess!:-)
   */
  void removeView( KonqChildView *view );

  void saveViewProfile( KConfig &cfg );
  void loadViewProfile( KConfig &cfg );
  void loadItem( KConfig &cfg, KonqFrameContainer *parent, const QString &name );

  void clear();

  void doGeometry( int width, int height );
  
  KonqChildView *chooseNextView( KonqChildView *view );
  unsigned long viewIdByNumber( int number );

  //bool isLinked( KonqChildView *source ) { return m_mapViewLinks.contains( source ); }
  
  //void createLink( KonqChildView *source, KonqChildView *destination );
  
  //KonqChildView * readLink( KonqChildView *view );
  
  //void removeLink( KonqChildView *source );
  
  //KonqChildView *linkableViewVertical( KonqChildView *view, bool above );
  //KonqChildView *linkableViewHorizontal( KonqChildView *view, bool left );
  
private:

  /**
   * Mainly creates the the backend structure(KonqChildView) for a view and
   * connects it
   */
  void setupView( KonqFrameContainer *parentContainer, Browser::View_ptr view, const QStringList &serviceTypes );
  
  KonqMainView *m_pMainView;
  
  KonqFrameContainer *m_pMainContainer;

  //QMap<KonqChildView *, KonqChildView*> m_mapViewLinks;
};

#endif
