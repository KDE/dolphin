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

#ifndef __kbrowser_h__
#define __kbrowser_h__

// constants used when dragging a selection rectange outside the browser window
#define AUTOSCROLL_DELAY	150
#define AUTOSCROLL_STEP		20

#define MAX_REQUEST_JOBS 2

#include <qpoint.h>
#include <qcolor.h>
#include <qlist.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>

#include <khtmlview.h>

#include <string>

class QPainter;
class KBrowser;

class KBrowserURLRequestJob : QObject
{
  Q_OBJECT
public:
  KBrowserURLRequestJob( KBrowser* _browser );
  ~KBrowserURLRequestJob();

  void run( QString _url, QString _simple_url, bool _reload );

signals:
  void error( const char *_url, int _err, const char* _errtext );
  
protected slots:
  void slotFinished( int _id );
  void slotData( int _id, const char*, int _len );
  void slotError( int _id, int _err, const char *_text );
  
protected:
  int m_jobId;
  KBrowser *m_pBrowser;
  QString m_strURL;
  QString m_strSimpleURL;
};

/**
 * If you derive from KBrowser you must overload the method @ref #createFrame
 */
class KBrowser : public KHTMLView
{
  Q_OBJECT

  friend KBrowserURLRequestJob;
  
public:
  KBrowser( QWidget *parent=0, const char *name=0, KBrowser *_parent_browser = 0L );
  virtual ~KBrowser();

  virtual void openURL( QString _url );
  virtual void openURL( QString _url, bool _reload, int _xoffset = 0, int _yoffset = 0, const char* _post_data = 0L );

  // conflict with KonqHtmlView  virtual const char* url() { return m_strURL; }
  
  virtual void setDefaultTextColors( const QColor& _textc, const QColor& _linkc, const QColor& _vlinkc );
  virtual void setDefaultBGColor( const QColor& bgcolor );

  /**
   * In Filemanager mode you get for example rubber band selection while
   * you have text selection otherwise.
   */
  virtual bool isFileManagerMode() { return m_bFileManagerMode; }
  virtual void setFileManagerMode( bool _m ) { m_bFileManagerMode = _m; }
  
public slots:
  virtual void slotStop();
  virtual void slotReload();
  virtual void slotReloadFrames();

signals:
  /**
   * Emitted when cancel is emitted, but with more detailed error
   * description.
   */
  void error( int _err, const char* _text );
  /**
   * Emitted if a link is pressed which has an invalid target, or the target <tt>_blank</tt>.
   */
  void newWindow( QString _url );

  void started( const char *_url );
  void completed();
  void canceled();
  void mousePressed( const char* _url, const QPoint& _point, int _button);

  void frameInserted( KBrowser *frame );
  void urlClicked( QString url );
  
protected slots:
  void slotFinished( int _id );
  void slotData( int _id, const char*, int _len );
  void slotError( int _id, int _err, const char *_text );
  void slotRedirection( int _id, const char *_url );

  void slotURLSelected( QString _url, int _button, QString _target );
  /**
   * Called if the user presses the submit button.
   *
   * @param _url is the <form action=...> value
   * @param _method is the <form method=...> value
   */
  virtual void slotFormSubmitted( QString _method, QString _url, const char *_data );
  virtual void slotUpdateSelect( int );

  /**
   * Overloads a method in @ref KHTMLView
   */
  virtual void slotURLRequest( QString _url );
  /**
   * Overloads a method in @ref KHTMLView
   */
  virtual void slotCancelURLRequest( QString _url );

  virtual void slotDocumentFinished( KHTMLView* _view );
  
protected:
  virtual void servePendingURLRequests();
  virtual void urlRequestFinished( KBrowserURLRequestJob* _request );

  /**
   * This function is used by @ref #newView. Its only purpose is to create
   * a new instance of this class. If you derived from KBrowser you must
   * overload this function to make shure that all frames are of the same
   * derived class.
   */
  virtual KBrowser* createFrame( QWidget *_parent, const char *_name );
  
  /**
   * For internal use only
   *
   * Overrides @ref KHTMLView::newView. It just creates a new instance
   * of KBrowser. These instances are used as frames.
   * Do NOT overload this function. Please overload @ref #createFrame.
   */
  virtual KHTMLView* newView( QWidget *_parent, const char *_name, int _flags );

  /**
   * We want to clear our list of child frames here.
   */
  virtual void begin( QString _url = 0L, int _x_offset = 0, int _y_offset = 0 );
    
  /**
   * @return the currently selected view ( the one with the black
   *         frame around it ) or 'this' if we dont have frames.
   *         Use this function only with the topmost KBrowser.
   *         This function will never return 0L.
   */
  KBrowser* activeView();

  /**
   * This function is hooked into the event processing of the @ref KHTMLWidget.
   *
   * @see KHTMLView::mouseMoveHook
   */
  virtual bool mouseMoveHook( QMouseEvent *_ev );
  /**
   * This function is hooked into the event processing of the @ref KHTMLWidget.
   *
   * @see KHTMLView::mouseReleasedHook
   */
  virtual bool mouseReleaseHook( QMouseEvent *_ev );
  /**
   * This function is hooked into the event processing of the @ref KHTMLWidget.
   *
   * @see KHTMLView::mousePressedHook
   */
  virtual bool mousePressedHook( QString _url, QString _target, QMouseEvent *_ev,
				 bool _isselected);
  
  virtual QString completeURL( QString _url );

  virtual KBrowser* findChildView( const char *_target );
  
  struct Child
  {
    Child( KBrowser *_b, bool _r ) { m_pBrowser = _b; m_bReady = _r; }
    
    KBrowser* m_pBrowser;
    bool m_bReady;
  };

  virtual void childCompleted( KBrowser* _browser );
  
  /**
   * A list containing all direct child views. Usually every frame in a frameset becomes
   * a child view. This list is used to update the contents of all children ( see @ref #slotUpdateView ).
   *
   * @see newView
   */
  QList<Child> m_lstChildren;
  
  /**
   * Upper left corner of a rectangular selection.
   */
  int rectX1, rectY1;
  /**
   * Lower right corner of a rectangular selection.
   */
  int rectX2, rectY2;
  /**
   * This flag is TRUE if we are in the middle of a selection using a
   * rectangular rubber band.
   */
  bool m_bStartedRubberBand;
  /**
   * This flag is true if the rubber band is currently visible.
   */
  bool m_bRubberBandVisible;
  /**
   * Painter used for the rubber band.
   */
  QPainter* m_pRubberBandPainter;    

  /**
   * This is just a temporary variable. It stores the URL the user clicked
   * on, until he releases the mouse again.
   *
   * @ref #mouseMoveHook
   * @ref #mousePressedHook
   */
  QString m_strSelectedURL;

  QString m_strURL;
  /**
   * Once we received the first data of our new HTML page we clear this
   * variable and copy its content to @ref #m_strURL
   */
  QString m_strWorkingURL;
  
  /**
   * One can pass offsets to @ref #openURL to indicate which part of the
   * HTML page should be visible. This variable is used to store the XOffset.
   * It is used when the method @ref #begin is called which happens once the
   * first data package of the new HTML page arrives.
   */
  int m_iNextXOffset;
  /**
   * One can pass offsets to @ref #openURL to indicate which part of the
   * HTML page should be visible. This variable is used to store the XOffset.
   * It is used when the method @ref #begin is called which happens once the
   * first data package of the new HTML page arrives.
   */
  int m_iNextYOffset;

  /**
   * This is the id of the job that fetches the HTML page for us.
   * A value of 0 indicates that we have no running job.
   */
  int m_jobId;

  bool m_bFileManagerMode;

  /**
   * This flag is set to true after calling the method @ref #begin and
   * before @ref #end is called.
   */
  bool m_bParsing;

  QStringList m_lstPendingURLRequests;
  QList<KBrowserURLRequestJob>  m_lstURLRequestJobs;
  
  KBrowser* m_pParentBrowser;

  /**
   * This flag is set to false after a call to @ref #openURL and set to true
   * once the document is parsed and displayed and if all children ( frames )
   * are complete, too. On completion the signal @ref #completed is emitted
   * and the parent browser ( @ref #m_pParentBrowser ) is notified.
   * If an fatal error occurs, this flag is set to true, too.
   */
  bool m_bComplete;

  /**
   * Tells wether the last call to @ref #openURL had the reload flag set or not.
   * This is needed to use the same cache policy for loading images and stuff.
   */
  bool m_bReload;
};

#endif

