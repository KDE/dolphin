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
#include "konq_mainwindow.h"

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
  /**
   * Create a child view
   * @param view the IDL View to be added in the child view
   * @param row the row (i.e. splitter) where to add the frame
   * @param newViewPosition only valid if Left or Right
   * @param parent the openparts parent for the view
   * @param mainWindow the KonqMainWindow hosting the view
   * @param mainView the KonqMainView owning the child view
   */
  KonqChildView( Konqueror::View_ptr view, Row * row,
                 Konqueror::NewViewPosition newViewPosition,
                 OpenParts::Part_ptr parent,
                 OpenParts::MainWindow_ptr mainWindow,
                 KonqMainView * mainView
               );
  ~KonqChildView();

  /* Get view's row */
  Row * getRow() { return m_row; }
  /* Attach a view */
  void attach( Konqueror::View_ptr view );
  /* Detach attached view, before deleting myself, or attaching another one */
  void detach();

  void repaint();

  /**
   * Changes the view mode of the current view, if different from viewName
   * @returns the new openparts 'id'
   */
  int changeViewMode( const char *viewName );

  /**
   * Create a view
   * @param viewName the type of view to be created (e.g. "KonqKfmIconView") 
   */
  Konqueror::View_ptr createViewByName( const char *viewName );

  //  bool mappingGotFocus( OpenParts::Part_ptr child );
  //  bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  //  virtual void openURL( const Konqueror::URLRequest &url );
  //  virtual void openURL( const char * _url, CORBA::Boolean _reload );

  // void makeHistory( View *v );
  
//public slots:  
  
protected:
  /**
   * Connects the internal View to the mainview. Do this after creating it and before inserting it
   */
  void connectView();

public: // temporary !!

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

protected:
  KonqMainView * m_mainView;
  OpenParts::Part_var m_vParent;
  OpenParts::MainWindow_var m_vMainWindow;
  KonqFrame *m_pFrame;
  Row * m_row;
};

#endif
