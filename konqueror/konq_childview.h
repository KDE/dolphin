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

#include <kparts/browserextension.h>

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
  QString locationBarURL; // can be different from url when showing a index.html
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
		 const KTrader::OfferList &partServiceOffers,
		 const KTrader::OfferList &appServiceOffers,
		 const QString &serviceType );

  ~KonqChildView();

  /** Force a repaint of the frame */
  void repaint();

  /** Show the view */
  void show();

  /**
   * Displays another URL, but without changing the view mode (caller has to
   * ensure that the call makes sense)
   */
  void openURL( const KURL &url );

  /**
   * Replace the current view vith _vView
   */
  void switchView( KonqViewFactory &viewFactory );

  /**
   * Change the type of view (i.e. loads a new konqueror view)
   * @param serviceType the service type we want to show
   * @param serviceName allows to enforce a particular service to be chosen,
   *        @see KonqFactory.
   * @param url the URL to open in the view. If not set, no URL is opened.
   * @param locationBarURL the url we want to display in the location bar
   *    May be different from @p url e.g. if using "allowHTML".
   */
  bool changeViewMode( const QString &serviceType,
                       const QString &serviceName = QString::null,
                       const KURL &url = KURL(),
                       const QString &locationBarURL = QString::null );

  /**
   * Call this to prevent next openURL() call from changing history lists
   * Used when the same URL is reloaded (for instance with another view mode)
   */
  void lockHistory() { m_bLockHistory = true; }

  /**
   * @return true if view can go back
   */
  bool canGoBack() { return m_lstHistory.at() != 0; }

  /**
   * @return true if view can go forward
   */
  bool canGoForward() { return m_lstHistory.at() != ((int)m_lstHistory.count())-1; }

  /**
   * Move in history. +1 is "forward", -1 is "back", you can guess the rest.
   */
  void go( int steps );

  const QList<HistoryEntry> & history() { return m_lstHistory; }

  /**
   * Set the KonqRun instance that is running something for this view
   * The mainview uses this to store the KonqRun for each child view.
   */
  void setRun( KonqRun * run  );
  /**
   * Stop loading
   */
  void stop();
  /**
   * Reload
   */
  void reload();

  /**
   * Retrieve view's URL
   */
  KURL url();

  /**
   * Get view's location bar URL, i.e. the one that the view signals
   * It can be different from url(), for instance if we display a index.html (David)
   */
  const QString locationBarURL() { return m_sLocationBarURL.stripWhiteSpace(); }

  /**
   * Get view object (should never be needed, except for IDL methods
   * like activeView() and viewList())
   */
  KParts::ReadOnlyPart *view() { return m_pView; }

  /**
   * see KonqViewManager::removePart
   */
  void partDeleted() { m_pView = 0L; }

  KParts::BrowserExtension *browserExtension() {
      return m_pView ? static_cast<KParts::BrowserExtension *>(m_pView->child( 0L, "KParts::BrowserExtension" )) : 0L ;
  }

  /**
   * Returns a pointer to the KonqFrame which the view lives in
   */
  KonqFrame* frame() { return m_pKonqFrame; }

  /**
   * Returns the servicetype this view is currently displaying
   */
  QString serviceType() { return m_serviceType; }

  /**
   * Returns the Servicetypes this view is capable to display
   */
  QStringList serviceTypes() { return m_service->serviceTypes(); }

  bool supportsServiceType( const QString &serviceType ) { return serviceTypes().contains( serviceType ); }

  // True if "Use index.html" is set (->the view doesn't necessarily show HTML!)
  void setAllowHTML( bool allow ) { m_bAllowHTML = allow; }
  bool allowHTML() const { return m_bAllowHTML; }

  // True if currently loading
  void setLoading( bool b ) { m_bLoading = b; }
  bool isLoading() const { return m_bLoading; }

  // True if "locked to current location" (and their view mode, in fact)
  bool passiveMode() const { return m_bPassiveMode; }
  void setPassiveMode( bool mode );

  // True if locked to current view mode (usually temporarily)
  bool lockedViewMode() const { return m_bLockedViewMode; }
  void setLockedViewMode( bool mode ) { m_bLockedViewMode = mode; }

  // True if 'link' symbol set
  bool linkedView() const { return m_bLinkedView; }
  void setLinkedView( bool mode );

  KService::Ptr service() { return m_service; }

  KTrader::OfferList partServiceOffers() { return m_partServiceOffers; }
  KTrader::OfferList appServiceOffers() { return m_appServiceOffers; }

  KonqMainView *mainView() const { return m_pMainView; }

  void initMetaView();
  void closeMetaView();

  void callExtensionMethod( const char *methodName );
  void callExtensionBoolMethod( const char *methodName, bool value );

  void setViewName( const QString &name ) { m_name = name; }
  QString viewName() const { return m_name; }

  QStringList frameNames() const;

  static QStringList childFrameNames( KParts::ReadOnlyPart *part );

  static KParts::BrowserHostExtension *hostExtension( KParts::ReadOnlyPart *part, const QString &name );

signals:

  /**
   * Signal the main view that our id changed (e.g. because of changeViewMode)
   */
  void sigViewChanged( KonqChildView *childView, KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView );

public slots:
  /**
   * Store location-bar URL in the child view
   * and updates the main view if this view is the current one
   */
  void setLocationBarURL( const QString & locationBarURL );

protected slots:
  // connected to the KROP's KIO::Job
  void slotStarted( KIO::Job * job );
  void slotCompleted();
  void slotCanceled( const QString & errMsg );
  void slotPercent( KIO::Job *, unsigned long percent );
  void slotSpeed( KIO::Job *, unsigned long bytesPerSecond );

  /**
   * Connected to the BrowserExtension
   */
  void slotSelectionInfo( const KFileItemList &items );
  void slotOpenURLNotify();

protected:
  /**
   * Connects the internal View to the mainview.
   * Do this after creating it and before inserting it.
   */
  void connectView();

  /**
   * Creates a new entry in the history.
   */
  void createHistoryEntry();

  /**
   * Updates the current entry in the history.
   */
  void updateHistoryEntry();

  void sendOpenURLEvent( const KURL &url );

  void setServiceTypeInExtension();

////////////////// protected members ///////////////

  KParts::ReadOnlyPart *m_pView;

  QString m_sLocationBarURL;

  /**
   * The full history (back + current + forward)
   * The current position in the history is m_lstHistory.current()
   */
  QList<HistoryEntry> m_lstHistory;

  KonqMainView *m_pMainView;
  bool m_bAllowHTML;
  QGuardedPtr<KonqRun> m_pRun;
  KonqFrame *m_pKonqFrame;
  bool m_bLoading;
  bool m_bPassiveMode;
  bool m_bLockedViewMode;;
  bool m_bLinkedView;
  KTrader::OfferList m_partServiceOffers;
  KTrader::OfferList m_appServiceOffers;
  KService::Ptr m_service;
  QString m_serviceType;
  QGuardedPtr<KParts::ReadOnlyPart> m_metaView;
  bool m_bLockHistory;
  QString m_name;
  bool m_bAborted;
};

#endif
