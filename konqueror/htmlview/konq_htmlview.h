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

#include <kbrowser.h>

#include <khtml.h>

#include <qpoint.h>

class KonqHTMLView;
class KonqSearchDialog;

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

class KonqHTMLView;

class KonqBrowser : public KHTMLWidget
{
  Q_OBJECT
public:
  KonqBrowser( KonqHTMLView *htmlView, const char *name );

  virtual void openURL( const QString &url, bool reload = false, int xOffset = 0, int yOffset = 0, const char *post_data = 0L );

private:
  KonqHTMLView *m_pHTMLView;
};

class KonqHTMLView : public BrowserView
{
  Q_OBJECT
  friend class KonqBrowser;
public:
  KonqHTMLView();
  virtual ~KonqHTMLView();

  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

  virtual void print();

  virtual void saveDocument();
  virtual void saveFrame();

  virtual void slotLoadImages();

  virtual void saveState( QDataStream &stream );
  virtual void restoreState( QDataStream &stream );

  virtual void configure();
//  virtual void openURL( QString _url, bool _reload, int _xoffset = 0, int _yoffset = 0, const char *_post_data = 0L);
/*
  virtual void can( bool &copy, bool &paste, bool &move );

  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const QCString & );
*/
public slots:
  void slotMousePressed( const QString &, const QPoint&, int );
  void slotFrameInserted( KHTMLWidget *frame );

protected slots:
  void slotShowURL( const QString &_url );
  void slotSetTitle( QString title );
  void viewDocumentSource();
  void viewFrameSource();
  void saveBackground();

  void slotDocumentRedirection( int, const char *url );
  void slotNewWindow( const QString &url );

    //void slotSearch();

//  void slotSelectionChanged();

protected:
  void initConfig();
  virtual void resizeEvent( QResizeEvent * );

  void openTxtView( const QString &url );

//  virtual KBrowser *createFrame( QWidget *_parent, const char *_name );

    //  virtual KHTMLEmbededWidget* newEmbededWidget( QWidget* _parent, const char *_name,
    //						const char *_src, const char *_type,
    //						int _marginwidth, int _marginheight,
    //						int _frameborder, bool _noresize );

private slots:
  void updateActions();

private:

  bool m_bAutoLoadImages;

  KonqBrowser *m_pBrowser;

    // KonqSearchDialog *m_pSearchDialog;

  QString m_strURL;

  KAction *m_paViewDocument;
  KAction *m_paViewFrame;
  KAction *m_paSaveBackground;
    //QAction *m_paSearch;

};

#endif
