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

#ifndef __kfm_browser_h__
#define __kfm_browser_h__

#include "kbrowser.h"

#include <khtmlembed.h>

#include <qpoint.h>

class KfmView;

class KfmEmbededFrame : public KHTMLEmbededWidget
{
  Q_OBJECT
public:
  KfmEmbededFrame( QWidget *_parent, int _frameborder, bool _allowresize );
  
  void setChild( QWidget *_widget );
  QWidget* child();
  
protected:
  virtual void resizeEvent( QResizeEvent *_ev );
  
  QWidget* m_pChild;
};

class KfmBrowser : public KBrowser
{
  Q_OBJECT  

public:
  KfmBrowser( QWidget *_parent, KfmView *_view, const char *_name = 0L, KBrowser *_parent_browser = 0L );
  virtual ~KfmBrowser();

public slots:
  virtual void slotMousePressed( const char*, const QPoint&, int );

protected slots:
  void slotNewWindow( const char *_url );
  void slotOnURL( const char *_url );
  
protected:

  virtual void initConfig();
  /**
   * For internal use only
   *
   * Overrides @ref KBrowser::createFrame. It just creates a new instance
   * of KfmBrowser. These instances are used as frames.
   */
  virtual KBrowser* createFrame( QWidget *_parent, const char *_name );

  virtual KHTMLEmbededWidget* newEmbededWidget( QWidget* _parent, const char *_name,
						const char *_src, const char *_type,
						int _marginwidth, int _marginheight,
						int _frameborder, bool _noresize );

  KfmView* m_pView;
};

#endif
