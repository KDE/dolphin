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

#include "konq_factory.h"

#include <qnamespace.h>
#include <qobject.h>
#include <qmap.h>
#include <qguardedptr.h>

#include <ktrader.h>

#include <kparts/partmanager.h>

class QString;
class QStringList;
class KConfig;
class KonqMainWindow;
class KonqFrameBase;
class KonqFrameContainer;
class KonqView;
class BrowserView;
class KActionMenu;

namespace KParts
{
  class ReadOnlyPart;
};

class KonqViewManager : public KParts::PartManager
{
  Q_OBJECT
public:
  KonqViewManager( KonqMainWindow *mainWindow );
  ~KonqViewManager();

  /**
   * Splits the view, depending on orientation, either horizontally or
   * vertically. The first of the resulting views will contain the initial
   * view, the other will be a new one, constructed from the given
   * Service Type and open the given URL.
   * If no Service Type was provided it takes the one from the current view.
   * Returns the newly created view or 0L if the view couldn't be created.
   */
  KonqView* splitView( Qt::Orientation orientation,
                       const KURL &url,
                       QString serviceType = QString::null,
                       const QString & serviceName = QString::null );

  /**
   * Does basically the same as splitView() but inserts the new view at the top
   * of the view tree.
   * Returns the newly created view or 0L if the view couldn't be created.
   */
  KonqView* splitWindow( Qt::Orientation orientation );

  /**
   * Do the actual splitting. The new View will be created from serviceType.
   * Returns the newly created view or 0L if the new view couldn't be created.
   */
  KonqView* split (KonqFrameBase* splitFrame,
                   Qt::Orientation orientation,
                   const QString &serviceType = QString::null,
                   const QString &serviceName = QString::null,
                   KonqFrameContainer **newFrameContainer = 0L );

  /**
   * Guess!:-)
   * Also takes care of setting another view as active if @p view was the active view
   */
  void removeView( KonqView *view );

  /**
   * Saves the current view layout to a config file.
   * Remove config file before saving, especially if saveURLs is false.
   * @param cfg the config file
   * @param saveURLs whether to save the URLs in the profile or not
   */
  void saveViewProfile( KConfig &cfg, bool saveURLs );

  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param cfg the config file
   */
  void loadViewProfile( KConfig &cfg );

  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param filename the name of the config file (under the profiles dir, if relative)
   */
  void loadViewProfile( const QString & filename );

  /**
   * Load the config entries for a view.
   * @param cfg the config file
   * ...
   * @param defaultURL the URL to use if the profile doesn't contain urls
   */
  void loadItem( KConfig &cfg, KonqFrameContainer *parent,
                 const QString &name, const KURL & defaultURL );

  void clear();

  KonqView *chooseNextView( KonqView *view );

  /**
   * Called whenever
   * - the total number of views changed
   * - the number of views in passive mode changed
   * The implementation takes care of showing or hiding the statusbar indicators
   */
  void viewCountChanged();

  void setProfiles( KActionMenu *profiles );

  void profileListDirty();

  KonqFrameContainer *mainContainer() const { return m_pMainContainer; }

  KonqMainWindow *mainWindow() const { return m_pMainWindow; }

  /**
   * Reimplemented from PartManager
   */
  void removePart( KParts::Part * part );

protected slots:
  void slotProfileDlg();

  void slotProfileActivated( int id );

  void slotProfileListAboutToShow();

protected:

  // Disabled - we do it ourselves
  virtual void setActiveInstance( KInstance * ) {}

private:

  /**
   * Creates a new View based on the given ServiceType. If serviceType is empty
   * it clones the current view.
   * Returns the newly created view.
   */
  KonqViewFactory createView( const QString &serviceType,
                              const QString &serviceName,
			      KService::Ptr &service,
			      KTrader::OfferList &partServiceOffers,
			      KTrader::OfferList &appServiceOffers );

  /**
   * Mainly creates the backend structure(KonqView) for a view and
   * connects it
   */
  KonqView *setupView( KonqFrameContainer *parentContainer,
                            KonqViewFactory &viewFactory,
		            const KService::Ptr &service,
		            const KTrader::OfferList &partServiceOffers,
			    const KTrader::OfferList &appServiceOffers,
			    const QString &serviceType );

  //just for debugging
  void printSizeInfo( KonqFrameBase* frame,
		      KonqFrameContainer* parent,
		      const char* msg );
  void printFullHierarchy( KonqFrameContainer * container, int ident = 0 );

  KonqMainWindow *m_pMainWindow;

  KonqFrameContainer *m_pMainContainer;

  QGuardedPtr<KActionMenu> m_pamProfiles;
  bool m_bProfileListDirty;

  QMap<QString, QString> m_mapProfileNames;
};

#endif
