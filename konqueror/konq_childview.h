/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#ifndef __konq_childview_h__
#define __konq_childview_h__

#include "konqueror.h"

#include <qlist.h>
#include <qstring.h>
#include <qobject.h>

#include <list>
#include <string>

class QSplitter;
class KonqFrame;

typedef QSplitter Row;

/* This class represents a child of the main view. The main view maintains
 * the list of children. A KonqChildView contains a Konqueror::View and
 * handles it. 
 */
class KonqChildView : public QObject
{
  Q_OBJECT
public:
  //C++
  KonqChildView();
  ~KonqChildView();

  //  bool mappingGotFocus( OpenParts::Part_ptr child );
  //  bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  //  virtual void openURL( const Konqueror::URLRequest &url );
  //  virtual void openURL( const char * _url, CORBA::Boolean _reload );
  
//public slots:  
  
// protected: TODO !!
  /* Changes the view mode of the current view, if different from viewName*/
  //  void changeViewMode( const char *viewName );
  /* Connects a view to the mainview. Do this after creating it and before inserting it */
  //  void connectView( Konqueror::View_ptr view );

  // void makeHistory( View *v );

  struct InternalHistoryEntry
  {
    bool bHasHistoryEntry;
    string strURL;
    Konqueror::View::HistoryEntry entry;
    CORBA::String_var strViewName;
  };
    
  bool m_bCompleted;
    
  Konqueror::View_var m_vView;
    
  /* ? */
  QString m_strLastURL;
    
  /* ? */
  QString m_strLocationBarURL;

  bool m_bBack;
  bool m_bForward;
  int m_iHistoryLock;
  
  InternalHistoryEntry m_tmpInternalHistoryEntry;
    
  list<InternalHistoryEntry> m_lstBack;
  list<InternalHistoryEntry> m_lstForward;
    
  KonqFrame *m_pFrame;
  Row * row;
};

#endif
