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

#ifndef __konq_htmlview_h__
#define __konq_htmlview_h__

#include "kbrowser.h"
#include "konq_baseview.h"

#include <khtmlembed.h>

#include <qpoint.h>

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
                     virtual public Konqueror::HTMLView_skel
{
  Q_OBJECT  

public:
  KonqHTMLView( QWidget *_parent = 0L, const char *_name = 0L, KBrowser *_parent_browser = 0L );
  virtual ~KonqHTMLView();

  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu );

  virtual void stop();
  virtual char *viewName() { return "KonquerorHTMLView"; }

  virtual char *url();
  virtual char *title();

  virtual void testIgnore();
    
public slots:
  virtual void slotMousePressed( const char*, const QPoint&, int );

protected slots:
  void slotOnURL( const char *_url );
  void slotSetTitle( const char *title );
  
protected:

  virtual void initConfig();

  /**
   * For internal use only
   *
   * Overrides @ref KBrowser::createFrame. It just creates a new instance
   * of KonqHTMLView. These instances are used as frames.
   */
  virtual KBrowser* createFrame( QWidget *_parent, const char *_name );

  virtual bool mousePressedHook( const char* _url, const char *_target, QMouseEvent *_ev,
				 bool _isselected);

  virtual KHTMLEmbededWidget* newEmbededWidget( QWidget* _parent, const char *_name,
						const char *_src, const char *_type,
						int _marginwidth, int _marginheight,
						int _frameborder, bool _noresize );

};

#endif
