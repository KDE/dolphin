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
#include "konq_factory.h"

#include <kbrowser.h>

#include <qlist.h>
#include <qstring.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qguardedptr.h>
#include <qcstring.h>

#include <ktrader.h>

class KonqRun;
class KonqFrame;

struct HistoryEntry
{
  KURL url;
  QByteArray buffer;
  QString strServiceType;
  QString strServiceName;
};

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
   * @param viewFrame the frame where to create the view - becomes owned by the view,
   * which will delete it when destroyed.
   * @param mainView is the mainview :-)
   * @param serviceTypes is the list of supported servicetypes
   */
  KonqChildView( KonqViewFactory &viewFactory,
		 KonqFrame* viewFrame,
		 KonqMainView * mainView,
		 const KService::Ptr &service,
		 const KTrader::OfferList &serviceOffers );

  ~KonqChildView();

  /** Force a repaint of the frame */
  void repaint();

  /** Show the view */
  void show();

  /**
   * Displays another URL, but without changing the view mode (caller has to
   * ensure that the call makes sense)
   */
  void openURL( const KURL &url, bool useMiscURLData = false );

  /**
   * Replace the current view vith _vView
   */
  void switchView( KonqViewFactory &viewFactory );

  bool changeViewMode( const QString &serviceType, const KURL &url = QString::null,
                       bool useMiscURLData = true, const QString &serviceName = QString::null );

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

  const QList<HistoryEntry> backHistory() { return m_lstBack; }
  const QList<HistoryEntry> forwardHistory() { return m_lstForward; }

  /**
   * Run the view (uses KonqRun)
   */
  void run( const KURL & url );
  /**
   * Stop loading
   */
  void stop();
  /**
   * Reload
   */
  void reload();

  /**
   * Get view's URL - slow method, avoid using it if possible
   */
  KURL url();

  /**
   * Get view's location bar URL, i.e. the one that the view signals
   * It can be different from url(), for instance if we display a index.html (David)
   */
  const QString locationBarURL() { return m_sLocationBarURL; }

  /**
   * Get view object (should never be needed, except for IDL methods
   * like activeView() and viewList())
   */
  KParts::ReadOnlyPart *view() { return m_pView; }

  KParts::BrowserExtension *browserExtension() { return (KParts::BrowserExtension *)m_pView->child( 0L, "KParts::BrowserExtension" ); }

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

  QString serviceType() { return m_serviceType; }

  /**
   * Returns the Servicetypes this view is capable to display
   */
  QStringList serviceTypes() { return m_service->serviceTypes(); }

  bool supportsServiceType( const QString &serviceType ) { return serviceTypes().contains( serviceType ); }

  void setMiscURLData( bool reload, int xOffset, int yOffset )
  {
    m_bReloadURL = reload;
    m_iXOffset = xOffset;
    m_iYOffset = yOffset;
  }
  bool reloadURL() const { return m_bReloadURL; }
  int xOffset() const { return m_iXOffset; }
  int yOffset() const { return m_iYOffset; }

  void setLoading( bool b ) { m_bLoading = b; }
  bool isLoading() const { return m_bLoading; }

  void setViewStarted( bool b ) { m_bViewStarted = b; }
  bool viewStarted() const { return m_bViewStarted; }

  void setProgress( int percent ) { m_iProgress = percent; }
  int progress() const { return m_iProgress; }

  bool passiveMode() const { return m_bPassiveMode; }
  void setPassiveMode( bool mode );

  bool supportsProgressIndication() const { return m_bProgressSignals; }

  KService::Ptr service() { return m_service; }

  KTrader::OfferList serviceOffers() { return m_serviceOffers; }

  KonqMainView *mainView() const { return m_pMainView; }

signals:

  /**
   * Signal the main view that our id changed (e.g. because of changeViewMode)
   */
  void sigViewChanged( KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView );

protected:
  /**
   * Connects the internal View to the mainview. Do this after creating it and before inserting it
   */
  void connectView();

////////////////// protected members ///////////////

  void go( QList<HistoryEntry> &stack, int steps );

  KParts::ReadOnlyPart *m_pView;

  QString m_sLocationBarURL;

  bool m_bBack;
  bool m_bForward;

  QList<HistoryEntry> m_lstBack;
  QList<HistoryEntry> m_lstForward;

  HistoryEntry *m_pCurrentHistoryEntry;

  /** If true, next call to makeHistory won't change the history */
  bool m_bHistoryLock;

  KonqMainView *m_pMainView;
  bool m_bAllowHTML;
  QGuardedPtr<KonqRun> m_pRun;
  bool m_bReloadURL;
  int m_iXOffset;
  int m_iYOffset;
  KonqFrame *m_pKonqFrame;
  bool m_bLoading;
  bool m_bViewStarted;
  int m_iProgress;
  bool m_bPassiveMode;
  bool m_bProgressSignals;
  KTrader::OfferList m_serviceOffers;
  KService::Ptr m_service;
  QString m_serviceType;
};

#endif
