/*
 *  This file is part of the KDE project
 *  Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/     

#ifndef __konq_childview_h__
#define __konq_childview_h__ "$Id$"

#include "konq_mainview.h"
#include "konq_viewmgr.h"

#include <qlist.h>
#include <qstring.h>
#include <qobject.h>
#include <qstringlist.h>

class KonqBaseView;
class KfmRun;
class KonqFrame;
class QSplitter;

/* This class represents a child of the main view. The main view maintains
 * the list of children. A KonqChildView contains a Browser::View and
 * handles it. It's more or less the backend structure for the views.
 * The widget handling stuff is done by the KonqFrame.
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
   * @param mainView is the mainview :-)
   * @param serviceTypes is the list of supported servicetypes
   */
  KonqChildView( Browser::View_ptr view,
		 KonqFrame* viewFrame,
		 KonqMainView * mainView,
		 const QStringList &serviceTypes );

  ~KonqChildView();

  /** Attach a view
   * @param view the view to attach (instead of the current one, if any)
   */
  void attach( Browser::View_ptr view );
  /** Detach attached view, before deleting myself, or attaching another one */
  void detach();

  /** Force a repaint of the frame */
  void repaint();

  /** Show the view */
  void show();

  /**
   * Displays another URL, but without changing the view mode (caller has to 
   * ensure that the call makes sense)
   */
  void openURL( const QString &url, bool useMiscURLData = false );

  /**
   * Replace the current view vith _vView
   */
  void switchView( Browser::View_ptr _vView, const QStringList &serviceTypes );

  bool changeViewMode( const QString &serviceType, const QString &url = QString::null,
                       bool useMiscURLData = true );
  void changeView( Browser::View_ptr _vView, const QStringList &serviceTypes, 
                   const QString &url = QString::null );
  
  /**
   * Create a view
   */
  Browser::View_ptr createViewByServiceType( const QString &serviceType );
  
  /**
   * Call this to prevent next makeHistory() call from changing history lists
   * This must be called before the first call to makeHistory().
   */
  void lockHistory() { m_bHistoryLock = true; }
  
  /**
   * Fills m_lstBack and m_lstForward - better comment needed, I'm clueless here (David)
   */
  void makeHistory( bool pushEntry );
    
  /**
   * @return true if view can go back
   */
  bool canGoBack() { return m_lstBack.count() != 0; }
  /**
   * Go back
   */
  void goBack( int steps = 1 );
  
  /**
   * @return true if view can go forward
   */
  bool canGoForward() { return m_lstForward.count() != 0; }
  /**
   * Go forward
   */
  void goForward( int steps = 1 );

  QStringList backHistoryURLs();
  QStringList forwardHistoryURLs();

  /**
   * Stop loading
   */
  void stop() { m_vView->stop(); }
  /**
   * Reload
   */
  void reload();

  /**
   * Get view's URL - slow method, avoid using it if possible
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
   *
   * note: as you can see this method does *not* call _duplicate, so take
   *       care of this when calling..
   */
  Browser::View_ptr view() { return m_vView; }

  /**
   * Returns a pointer to the KonqFrame which the view lives in
   */
  KonqFrame* frame() { return m_pKonqFrame; }

  /**
   * Set location bar URL (called by MainView, when View signals it)
   */
  void setLocationBarURL( const QString locationBarURL ) { m_sLocationBarURL = locationBarURL; }

  void setAllowHTML( bool allow ) { m_bAllowHTML = allow; }
  bool allowHTML() const { return m_bAllowHTML; }

  /**
   * Returns the Servicetypes this view is capable to display
   */
  QStringList serviceTypes() { return m_lstServiceTypes; }
  
  bool supportsServiceType( const QString &serviceType );

  void setKfmRun( KfmRun *run ) { m_pRun = run; }
  KfmRun *kfmRun() const { return m_pRun; }

  void setMiscURLData( bool reload, int xOffset, int yOffset )
  {
    m_bReloadURL = reload;
    m_iXOffset = xOffset;
    m_iYOffset = yOffset;
  }
  bool reloadURL() const { return m_bReloadURL; }
  int xOffset() const { return m_iXOffset; }
  int yOffset() const { return m_iYOffset; }

  void setRowInfo( KonqViewManager::RowInfo *rowInfo ) { m_pRow = rowInfo; }
  KonqViewManager::RowInfo *rowInfo() const { return m_pRow; }

  static bool createView( const QString &serviceType, 
                          Browser::View_var &view, 
			  QStringList &serviceTypes, 
			  KonqMainView *mainView );

signals:

  /**
   * Signal the main view that our id changed (e.g. because of changeViewMode)
   */
  void sigIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId );

protected:
  /**
   * Connects the internal View to the mainview. Do this after creating it and before inserting it
   */
  void connectView();

////////////////// protected members ///////////////

  struct HistoryEntry
  {
    QString strURL;
    int xOffset;
    int yOffset;
    QString strServiceType;
  };

  void go( QList<HistoryEntry> &stack, int steps );
  
  Browser::View_var m_vView;
    
  QString m_sLocationBarURL;

  bool m_bBack;
  bool m_bForward;
  
  QList<HistoryEntry> m_lstBack;
  QList<HistoryEntry> m_lstForward;

  HistoryEntry *m_pCurrentHistoryEntry;

  /** If true, next call to makeHistory won't change the history */
  bool m_bHistoryLock;
    
  KonqMainView *m_pMainView;
  OpenParts::MainWindow_var m_vMainWindow;
  QStringList m_lstServiceTypes;
  bool m_bAllowHTML;
  KfmRun *m_pRun;
  bool m_bReloadURL;
  int m_iXOffset;
  int m_iYOffset;
  KonqFrame* m_pKonqFrame;
  KonqViewManager::RowInfo *m_pRow;
};

#endif
