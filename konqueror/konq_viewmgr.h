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
#include <qlist.h>

class QSplitter;
class QStringList;
class KConfig;
class KonqMainView;
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
  
  struct RowInfo
  {
    QList<KonqChildView> children;
    QSplitter *splitter;
  };


  void saveViewProfile( KConfig &cfg );
  void loadViewProfile( KConfig &cfg );

  void insertView( Qt::Orientation orientation,
		  Browser::View_ptr newView,
		  const QStringList &newViewServiceTypes );

  void removeView( KonqChildView *view );

  void clear();

  void doGeometry( int width, int height );
  
  KonqChildView *chooseNextView( KonqChildView *view );
  
private:

  void clearRow( RowInfo *row );

  void setupView( RowInfo *row, Browser::View_ptr view, const QStringList &serviceTypes );
  
  KonqMainView *m_pMainView;
  
  QSplitter *m_pMainSplitter;

  QList<RowInfo> m_lstRows;
};

#endif
