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

#ifndef __konq_main_h__
#define __konq_main_h__

#include <opApplication.h>

#include "konqueror.h"
#include "kbookmark.h"

class KonqMainWindow;
class KonqMainView;

class KonqApp : public OPApplication
{
  Q_OBJECT
public:
  KonqApp( int &argc, char** argv );
  ~KonqApp();
  
  virtual void start();  
};

class KonqApplicationIf : virtual public OPApplicationIf,
			  virtual public Konqueror::Application_skel
{
public:
  KonqApplicationIf();
  KonqApplicationIf( const CORBA::BOA::ReferenceData &refdata );
  KonqApplicationIf( CORBA::Object_ptr _obj );

  static KonqMainView   *allocMainView( const char *url = 0L );
  static KonqMainWindow *allocMainWindow( const char *url = 0L );

  virtual OpenParts::Part_ptr createPart();
  virtual OpenParts::MainWindow_ptr createWindow();

  Konqueror::KfmIconView_ptr createKfmIconView();
  Konqueror::HTMLView_ptr createHTMLView();
  Konqueror::KfmTreeView_ptr createKfmTreeView();
  Konqueror::TxtView_ptr createTxtView();
};

class KonqBrowserFactory : public Browser::BrowserFactory_skel
{
public:
  KonqBrowserFactory( const CORBA::BOA::ReferenceData &refData );
  KonqBrowserFactory( CORBA::Object_ptr obj );
  
  OpenParts::MainWindow_ptr createBrowserWindow( const QCString &url );
  OpenParts::Part_ptr createBrowserPart( const QCString &url );
};

class KonqBookmarkManager : public KBookmarkManager
{
public:
  KonqBookmarkManager( QString path ) : KBookmarkManager ( path ) {}
  ~KonqBookmarkManager() {}
  virtual void editBookmarks( const char *_url );
};

#endif
