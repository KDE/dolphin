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

#include <qlist.h>
#include <qstring.h>
#include <qobject.h>
#include <qstringlist.h>

class QVBoxLayout;
class QSplitter;
class KonqBaseView;
class OPFrame;
class KonqFrameHeader;

typedef QSplitter Row;

/* This class represents a child of the main view. The main view maintains
 * the list of children. A KonqChildView contains a Konqueror::View and
 * handles it. 
 *
 * KonqChildView makes the difference between built-in views and remote ones.
 * We create a widget, and a layout in it (with the FrameHeader as top item in the layout)
 * For builtin views we have the view as direct child widget of the layout
 * For remote views we have an OPFrame, having the view attached, as child widget of the layout
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
   * @param parentWidget the mainview as a widget
   * @param mainWindow the KonqMainWindow hosting the view
   */
  KonqChildView( Konqueror::View_ptr view,
                 Row * row,
                 NewViewPosition newViewPosition,
		 KonqMainView * mainView,
		 const QStringList &serviceTypes
               );

  ~KonqChildView();

  /** Get view's row */
  Row * row() { return m_row; }
  /** Attach a view
   * @param view the view to attach (instead of the current one, if any)
   * @param builtin if the view is a builtin one, the widget pointer
   */
  void attach( Konqueror::View_ptr view );
  /** Detach attached view, before deleting myself, or attaching another one */
  void detach();

  /** Force a repaint of the frame header */
  void repaint();

  /** Show the view */
  void show();

  /**
   * Displays another URL, but without changing the view mode (caller has to 
   * ensure that the call makes sense)
   */
  void openURL( QString url );
  /**
   * Builds or destroys view-specific part of the menus.
   */
  void emitMenuEvents( OpenPartsUI::Menu_ptr viewMmenu, OpenPartsUI::Menu_ptr editMenu, bool create );

  /**
   * Replace the current view vith _vView
   */
  void switchView( Konqueror::View_ptr _vView, const QStringList &serviceTypes );

  bool changeViewMode( const QString &serviceType, const QString &url = QString::null );
  
  void changeView( Konqueror::View_ptr _vView, const QStringList &serviceTypes, const QString &url = QString::null );
  
  /**
   * Create a view
   */
  Konqueror::View_ptr createViewByServiceType( const QString &serviceType );
  
  /**
   * Call this to prevent next makeHistory() call from changing history lists
   * This must be called before the first call to makeHistory().
   */
  void lockHistory() { m_bHistoryLock = true; }
  
  /**
   * Fills m_lstBack and m_lstForward - better comment needed, I'm clueless here (David)
   * @param bCompleted true if view has finished loading - hum.
   * @param url the current url.
   */
  void makeHistory( bool bCompleted, QString url );
    
  /**
   * @return true if view can go back
   */
  bool canGoBack() { return m_lstBack.count() != 0; }
  /**
   * Go back
   */
  void goBack();
  
  /**
   * @return true if view can go forward
   */
  bool canGoForward() { return m_lstForward.count() != 0; }
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
   */
  Konqueror::View_ptr view() { return Konqueror::View::_duplicate(m_vView); }
  // FIXME : is duplicated needed ? activeView will do it too !

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

  static bool createView( const QString &serviceType, 
                          Konqueror::View_var &view, 
			  QStringList &serviceTypes, 
			  KonqMainView *mainView );

signals:

  /**
   * Signal the main view that our id changed (e.g. because of changeViewMode)
   */
  void sigIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId );

public slots:
  /**
   * Called when the view header is clicked
   */
  void slotHeaderClicked();

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
    QString strServiceType;
  };

  Konqueror::View_var m_vView;
    
  QString m_sLocationBarURL;

  bool m_bBack;
  bool m_bForward;
  
  QValueList<InternalHistoryEntry> m_lstBack;
  QValueList<InternalHistoryEntry> m_lstForward;

  /** Used by makeHistory, to store an history entry between calls */
  InternalHistoryEntry m_tmpInternalHistoryEntry;
  /** If true, next call to makeHistory won't change the history */
  bool m_bHistoryLock;
    
  KonqMainView *m_pMainView;
  OpenParts::MainWindow_var m_vMainWindow;
  OPFrame *m_pFrame;
  QWidget *m_pWidget;
  KonqFrameHeader * m_pHeader;
  Row * m_row;
  QVBoxLayout * m_pLayout;
  QStringList m_lstServiceTypes;
  bool m_bAllowHTML;
};

#endif
