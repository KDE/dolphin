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

#include "konq_baseview.h"

#include <khtmlembed.h>
#include <kbrowser.h>

#include <qpoint.h>

class KonqMainView;
class KonqHTMLView;

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

class KonqHTMLView : public KBrowser,
                     public KonqBaseView,
                     virtual public Konqueror::HTMLView_skel,
		     virtual public Browser::PrintingExtension_skel,
		     virtual public Browser::EditExtension_skel
{
  Q_OBJECT  

public:
  KonqHTMLView( KonqMainView *mainView = 0L, KBrowser *parentBrowser = 0L, const char *name = 0L );
  virtual ~KonqHTMLView();

  virtual bool event( const char *event, const CORBA::Any &value );
  virtual bool mappingOpenURL( Browser::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu );
  virtual bool mappingFillMenuEdit( Browser::View::EventFillMenu_ptr editMenu );
  virtual bool mappingFillToolBar( Browser::View::EventFillToolBar viewToolBar );

  virtual void stop();

  virtual char *url();
  virtual CORBA::Long xOffset();
  virtual CORBA::Long yOffset();

  virtual void print();
  
  virtual void saveDocument();
  virtual void saveFrame();
  virtual void saveBackground();
  virtual void viewDocumentSource();
  virtual void viewFrameSource();
  void openTxtView( const QString &url );
  
  virtual void slotLoadImages();
  
  virtual void beginDoc( const char *url, CORBA::Long dx, CORBA::Long dy );
  virtual void writeDoc( const char *data );
  virtual void endDoc();
  virtual void parseDoc();

  virtual void openURL( QString _url, bool _reload, int _xoffset = 0, int _yoffset = 0, const char *_post_data = 0L);
  
  virtual void can( CORBA::Boolean &copy, CORBA::Boolean &paste, CORBA::Boolean &move );
  
  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const char * );
      
public slots:
  virtual void slotMousePressed( const QString &, const QPoint&, int );
  void slotFrameInserted( KBrowser *frame );

protected slots:
  void slotShowURL( KHTMLView *view, QString _url );
  void slotSetTitle( QString title );
  void slotStarted( const QString &url );
  void slotCompleted();
  void slotCanceled();
  
  void slotDocumentRedirection( int, const char *url );
  
  void slotNewWindow( const QString &url );
  
  void slotSelectionChanged();
  
protected:

//  virtual KBrowser *createFrame( QWidget *_parent, const char *_name );

  virtual KHTMLEmbededWidget* newEmbededWidget( QWidget* _parent, const char *_name,
						const char *_src, const char *_type,
						int _marginwidth, int _marginheight,
						int _frameborder, bool _noresize );

private:
  void checkViewMenu();
  OpenPartsUI::Menu_var m_vViewMenu;
  KonqMainView *m_pMainView;
  bool m_bAutoLoadImages;
  
  CORBA::Long m_idSaveDocument;
  CORBA::Long m_idSaveFrame;
  CORBA::Long m_idSaveBackground;
  CORBA::Long m_idViewDocument;
  CORBA::Long m_idViewFrame;
};

#endif
