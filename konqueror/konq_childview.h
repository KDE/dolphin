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
   * @param parent the mainview, parent of this view
   * @param mainWindow the KonqMainWindow hosting the view
   * @param mainView the KonqMainView owning the child view
   */
  KonqChildView( Konqueror::View_ptr view, Row * row,
                 Konqueror::NewViewPosition newViewPosition,
                 OpenParts::Part_ptr parent,
                 OpenParts::MainWindow_ptr mainWindow
               );
  ~KonqChildView();

  /** Get view's row */
  Row * getRow() { return m_row; }
  /** Attach a view */
  void attach( Konqueror::View_ptr view );
  /** Detach attached view, before deleting myself, or attaching another one */
  void detach();

  /** Force a repaint of the frame header */
  void repaint();

  /**
   * Displays another URL, but without changing the view mode (caller has to 
   * ensure that the call makes sense)
   */
  void openURL( QString url );
  /**
   * Builds or destroys view-specific part of the view menu.
   */
  void emitEventViewMenu( OpenPartsUI::Menu_ptr menu, bool create );

  /**
   * Changes the view mode of the current view, if different from viewName
   */
  void changeViewMode( const char *viewName );
  /**
   * Replace the current view vith _vView
   */
  void switchView( Konqueror::View_ptr _vView );

  /**
   * Create a view
   * @param viewName the type of view to be created (e.g. "KonqKfmIconView") 
   */
  Konqueror::View_ptr createViewByName( const char *viewName );
  
  /**
   * Call this to prevent next makeHistory() call from changing history lists
   * This must be called before the first call to makeHistory().
   */
  void lockHistory() { m_bHistoryLock = true; }
  
  /**
   * Fills m_lstBack and m_lstForward - better comment needed, I'm clueless here (David)
   * @param bCompleted true if view has finished loading - hum.
   * @param url the current url. Will be used on NEXT call to makeHistory. @see m_strLastURL
   */
  void makeHistory( bool bCompleted, QString url );
    
  /**
   * @return true if view can go back
   */
  bool canGoBack() { return m_lstBack.size() != 0; }
  /**
   * Go back
   */
  void goBack();
  
  /**
   * @return true if view can go forward
   */
  bool canGoForward() { return m_lstForward.size() != 0; }
  /**
   * Go forward
   */
  void goForward();

  /**
   * Stop loading
   */
  void stop() { m_vView->stop(); }
  /**
   * Reload
   */
  void reload();

  /**
   * Get view's URL
   */
  QString url();
  /**
   * Get view's name
   */
  QString viewName();
  /**
   * Get view's id
   */
  OpenParts::Id id() { return m_vView->id(); }
  /**
   * Get view's location bar URL, i.e. the one that the view signals
   * It can be different from url(), for instance if we display a index.html (David)
   */
  const QString locationBarURL() { return m_sLocationBarURL; }

  /**
   * Get view object (should never be needed, except for IDL methods 
   * like activeView() and viewList())
   */
  Konqueror::View_ptr getView() { return Konqueror::View::_duplicate(m_vView); }
  // FIXME : is duplicated needed ? activeView will do it too !

  /**
   * Set location bar URL (called by MainView, when View signals it)
   */
  void setLocationBarURL( const QString locationBarURL ) { m_sLocationBarURL = locationBarURL; }

signals:

  /**
   * Signal the main view that our id changed (e.g. because of changeViewMode)
   */
  void sigIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId );
  /**
   * Signal the main view that our URL changed, so it may have to change the state of the 'up' button
   */
  void sigSetUpEnabled( QString url, OpenParts::Id id );

protected:
  /**
   * Connects the internal View to the mainview. Do this after creating it and before inserting it
   */
  void connectView();

////////////////// protected members ///////////////

  struct InternalHistoryEntry
  {
    bool bHasHistoryEntry;
    QString strURL;
    Konqueror::View::HistoryEntry entry;
    CORBA::String_var strViewName;
  };

  /* Used by makeHistory, to store the URL _previously_ opened in this view */
  QString m_strLastURL;
    
  Konqueror::View_var m_vView;
    
  QString m_sLocationBarURL;

  bool m_bBack;
  bool m_bForward;
  
  list<InternalHistoryEntry> m_lstBack;
  list<InternalHistoryEntry> m_lstForward;

  /** Used by makeHistory, to store an history entry between calls */
  InternalHistoryEntry m_tmpInternalHistoryEntry;
  /** If true, next call to makeHistory won't change the history */
  bool m_bHistoryLock;
    
  OpenParts::Part_var m_vParent;
  OpenParts::MainWindow_var m_vMainWindow;
  KonqFrame *m_pFrame;
  Row * m_row;
};

#endif
