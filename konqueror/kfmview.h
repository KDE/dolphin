/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#ifndef __kfm_view_h__
#define __kfm_view_h__

#include <sys/stat.h>

#include <qwidget.h>
#include <qwidgetstack.h>
#include <qpoint.h>
#include <qstrlist.h>
#include <qpopupmenu.h>

#include "kservices.h"

#include "kbookmark.h"
#include "kmimetypes.h"
#include "kfmpopup.h"

#include <list>
#include <string>
#include <map>

#include <sys/types.h>

class QPixmap;
class KfmFinder;
class KfmIconView;
class KfmGui;
class KfmBrowser;
class KfmRun;
class KfmAbstractGui;

class KfmView : public QWidgetStack, public KBookmarkOwner
{
  friend KfmRun;
  
  Q_OBJECT
public:
  enum ViewMode { FINDER, HOR_ICONS, VERT_ICONS, HTML, NOMODE };
  
  KfmView( KfmAbstractGui *_gui, QWidget* _parent );
  virtual ~KfmView();

  KfmAbstractGui* gui() { return m_pGui; }

  /**
   * Shows a different view. Keep in mind that this is not everytime the user selected view.
   * The user may have selected "Tree View" as default view, but for HTML we will still
   * use HTML view.
   *
   * @param _open_url determines wether the current URL should be displayed in the new view.
   *                  This is sometimes unwanted since it may happen that view and URL change
   *                  at the same time. In these cases you may want to set this flag to false.
   */
  virtual void setViewMode( ViewMode _mode, bool _open_url = true );
  virtual ViewMode viewMode() { return m_viewMode; }
  
  virtual void openURL( const char *_url, mode_t _mode = 0, bool _is_local_file = false,
			int _xoffset = 0, int _yoffset = 0 );

  virtual void popupMenu( const QPoint &_global, QStrList& _urls, mode_t _mode = 0, bool _is_local_file = false );

  /**
   * If the view is currently loading an URL then the action is
   * stopped immediately.
   */
  virtual void stop();

  virtual bool isHTMLAllowed() { return m_bHTMLAllowed; }
  virtual void setHTMLAllowed( bool _allow );
  
  virtual QString workingURL() { return QString( m_strWorkingURL.c_str() ); }

  virtual void setFocus();
  virtual bool hasFocus() { return m_hasFocus; }

  virtual void reload();
  
  virtual void clearFocus();

  ////////////////////
  /// Overloaded functions of KBookmarkOwner
  ////////////////////
  /**
   * This function is called if the user selectes a bookmark. You must overload it.
   */
  virtual void openBookmarkURL( const char *_url );
  /**
   * @return the title of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentTitle();
  /**
   * @return the URL of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentURL();
 
signals:
  void canceled();
  void completed();
  void started();

  void upURL();
  void backHistory();
  void forwardHistory();

  void gotFocus( KfmView* view );

  void error( int _err, const char* _text );
  
protected slots:
  virtual void slotPopupCd();
  virtual void slotPopupNewView();
  virtual void slotPopup( int _id );
  virtual void slotPopupCopy();
  virtual void slotPopupPaste();
  virtual void slotPopupDelete();
  virtual void slotPopupOpenWith();
  
  virtual void slotURLStarted( const char *_url );
  virtual void slotURLCompleted();
  virtual void slotURLCanceled();

  virtual void slotDirectoryDirty( const char * _dir );
  virtual void slotDirectoryDeleted( const char * _dir );

  virtual void slotUp() { emit upURL(); }
  virtual void slotBack() { emit backHistory(); }
  virtual void slotForward() { emit forwardHistory(); }
  
protected:
  virtual void initConfig();
  virtual void resizeEvent( QResizeEvent *_ev );

  //////////////////////
  // Called from @ref KfmRun
  //////////////////////
  virtual void openDirectory( const char *_url );
  virtual void openHTML( const char *_url );
  virtual void openNothing();

  virtual void makeHistory();

  QString findIndexFile( const char *_path );
  
  virtual void focusInEvent( QFocusEvent* _event );

  KfmFinder* m_pFinder;
  KfmIconView* m_pIconView;
  KfmBrowser* m_pBrowser;
  ViewMode m_viewMode;

  KfmAbstractGui* m_pGui;

  /**
   * This variable is set if @ref #slotURLCompleted is called.
   */
  string m_strURL;
  string m_strWorkingURL;
  int m_iWorkingXOffset;
  int m_iWorkingYOffset;
  
  KfmRun* m_pRun;

  bool m_bHTMLAllowed;
  
  QStrList m_lstPopupURLs;
  map<int,KService*> m_mapPopup;
  map<int,KDELnkMimeType::Service> m_mapPopup2;

  QPopupMenu* m_popupMenu;
  KNewMenu* m_menuNew;

  bool m_hasFocus;
};

#endif
