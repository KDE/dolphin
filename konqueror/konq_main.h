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

#ifndef __konq_main_h__
#define __konq_main_h__

#include <opApplication.h>

#include "konqueror.h"
#include "kbookmark.h"

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

  OpenParts::Part_ptr createPart();
  OpenParts::MainWindow_ptr createWindow();
  
  Konqueror::MainView_ptr createMainView();
  
  Konqueror::KfmIconView_ptr createKfmIconView();
  Konqueror::HTMLView_ptr createHTMLView();
  Konqueror::KfmTreeView_ptr createKfmTreeView();
  Konqueror::PartView_ptr createPartView();
};

class KonqBookmarkManager : public KBookmarkManager
{
public:
  KonqBookmarkManager( QString path ) : KBookmarkManager ( path ) {}
  ~KonqBookmarkManager() {}
  virtual void editBookmarks( const char *_url );
};

#endif
