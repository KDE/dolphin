/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __konq_htmlview_h__
#define __konq_htmlview_h__

#include <kparts/browserextension.h>

#include <khtml.h>

#include <qpoint.h>

class KonqSearchDialog;
class KAction;
class KonqHTMLViewExtension;

/* ### FIXME (Lars)
class KonqEmbededFrame : public KHTMLEmbededWidget
{
  Q_OBJECT
public:
  KonqEmbededFrame( QWidget *_parent, int _frameborder, bool _allowresize );

  void setChild( QWidget *_widget );
  QWidget* child();

protected:
  virtual void resizeEvent( QResizeEvent *_ev );

  QWidget* m_pChild;
};
*/

/**
 *
 */
class KonqHTMLWidget : public KHTMLWidget
{
  Q_OBJECT
public:
  KonqHTMLWidget( QWidget * parent, const char *name );
  virtual ~KonqHTMLWidget() {}

  virtual void openURL( const QString &url, bool reload = false, int xOffset = 0, int yOffset = 0, const char *post_data = 0L );

  void initConfig();

signals:
  // ok, this is a bit dirty. Our widget emits signals with the exact same signature as the
  // BrowserExtension ones, so that they get connected directly.
   void openURLRequest( const KURL &url, KParts::URLArgs urlArgs );
   // already in parent void onURL( const QString & url );
   // already in parent void createNewWindow( const QString &url );
   void popupMenu( const QPoint &_global, const KFileItemList &_items );

public slots:
  // small hack to get popupmenus. Fix after krash
  void slotRightButtonPressed( const QString &_url, const QPoint &_global);
  void slotMousePressed( const QString &, const QPoint&, int );
  void slotFrameInserted( KHTMLWidget *frame );

//void slotSearch();
//void slotSelectionChanged();

protected:

//  virtual KBrowser *createFrame( QWidget *_parent, const char *_name );

    //  virtual KHTMLEmbededWidget* newEmbededWidget( QWidget* _parent, const char *_name,
    //						const char *_src, const char *_type,
    //						int _marginwidth, int _marginheight,
    //						int _frameborder, bool _noresize );

private:
  bool m_bAutoLoadImages;
    // KonqSearchDialog *m_pSearchDialog;
};

/**
 * The konqueror view, i.e. the part.
 */
class KonqHTMLView : public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
  KonqHTMLView( QWidget * parent = 0, const char *name = 0 );
  virtual ~KonqHTMLView();

  virtual bool openURL( const KURL &url );

  // we reimplement openURL so this is never called
  virtual bool openFile() { return false; }

  // used to be called stop()
  virtual bool closeURL();

  // to avoid having to call widget() and cast very often
  KonqHTMLWidget * htmlWidget() { return m_pWidget; }

  virtual void setXYOffset( int x, int y )
    { m_iXOffset = x; m_iYOffset = y; }

protected slots:
  void slotStarted( const QString & );
  void slotCompleted();
  void slotCanceled();
  void slotShowURL( const QString & _url );
  void slotSetTitle( const QString & title );
  void slotDocumentRedirection( int, const char *url );
  void saveDocument();
  void saveFrame();
  void saveBackground();
  void viewDocumentSource();
  void viewFrameSource();
  void slotLoadImages();

  void updateActions();

protected:
  void openTxtView( const KURL &url );

  int m_iXOffset;
  int m_iYOffset;

  KAction *m_paViewDocument;
  KAction *m_paViewFrame;
  KAction *m_paSaveBackground;
  KAction *m_paSaveDocument;
  KAction *m_paSaveFrame;
  //KAction *m_paSearch;

  KonqHTMLWidget * m_pWidget;
  KonqHTMLViewExtension * m_extension;
};

/**
 * The browser-extension that allows the view to be embedded in konqueror
 */
class KonqHTMLViewExtension : public KParts::BrowserExtension
{
  Q_OBJECT
    friend class KonqHTMLView; // emits our signals
public:
  KonqHTMLViewExtension( KonqHTMLView *view, const char *name = 0 );
  virtual ~KonqHTMLViewExtension() {}

  virtual void setXYOffset( int x, int y )
    {
      m_pView->setXYOffset( x, y );
    }
  virtual int xOffset();
  virtual int yOffset();

public slots:
  // Automatically detected by konqueror
  // TODO add cut, copy, pastecopy, pastecut, ...
  virtual void print();
  virtual void reparseConfiguration();
  virtual void saveLocalProperties();
  virtual void savePropertiesAsDefault();
  virtual void saveState( QDataStream &stream );
  virtual void restoreState( QDataStream &stream );

private:
  KonqHTMLView * m_pView;
};

#endif
