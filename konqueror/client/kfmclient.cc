/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>
 
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

// $Id$

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <fstream.h>

#include <qdir.h>
#include <qmessagebox.h>
#include <qstring.h>

#include <kpropsdlg.h>
#include <krun.h>
#include <kregistry.h>
#include <kregfactories.h>
#include <kservices.h>
#include <kded_instance.h>
#include <knaming.h>
#include <kactivator.h>
#include <ktrader.h>
#include <openparts.h>

#include <cutesti.h>

#include "kfmclient.h"

int main( int argc, char **argv )
{
  if ( argc == 1 )
  {
    // Should all the following be i18n'ed ?
    printf("\nSyntax:\n");
    printf("  kfmclient openURL\n"
           "            # Opens a dialog to ask you for the URL\n\n");
    printf("  kfmclient openURL 'url'\n"
           "            # Opens a window showing 'url'. If such a window\n");
    printf("            #   exists, it is shown. 'url' may be \"trash:/\"\n"
           "            #   to open the trash bin.\n\n");
    printf("  kfmclient refreshDesktop\n"
           "            # Refreshes the desktop (obsolete)\n\n");
    printf("  kfmclient refreshDirectory 'url'\n"
           "            # Tells KFM that an URL has changes. If KFM\n");
    printf("            #   is displaying that URL, it will be reloaded. (obsolete)\n\n");
    printf("  kfmclient openProperties 'url'\n"
           "            # Opens a properties menu\n\n");
    printf("  kfmclient exec 'url' ['binding']\n"
           "            # Tries to execute 'url'. 'url' may be a usual\n"
           "            #   URL, this URL will be opened. You may omit\n"
           "            #   'binding'. In this case the default binding\n");
    printf("            #   is tried. Of course URL may be the URL of a\n"
           "            #   document, or it may be a *.desktop file.\n");
    printf("            #   This way you could for example mount a device\n"
           "            #   by passing 'Mount default' as binding to \n"
           "            #   'cdrom.desktop'\n\n");
    printf("  kfmclient move 'src' 'dest'\n"
           "            # Copies the URL 'src' to 'dest'.\n"
           "            #   'src' may be a list of URLs.\n");
    printf("            #   'dest' may be \"trash:/\" to move the files\n"
           "            #   in the trash bin.\n\n");
    printf("  kfmclient folder 'src' 'dest'\n"
           "            # Like move if 'src' is given,\n"
           "            #   otherwise like openURL src \n\n");
    printf("  kfmclient sortDesktop\n"
           "            # Rearranges all icons on the desktop.\n\n");
    printf("  kfmclient selectDesktopIcons x y w h add\n"
           "            # Selects the icons on the desktop in the given rectangle\n"
           "            # If add is 1, adds selection to the current one\n");
    printf("  kfmclient configure\n"
           "            # Re-read KFM's configuration.\n\n");
    printf("*** Examples:\n"
           "  kfmclient exec file:/usr/local/kde/bin/kdehelp Open\n"
           "             // Starts kdehelp\n\n");
    printf("  kfmclient exec file:/root/Desktop/cdrom.desktop \"Mount default\"\n"
           "             // Mounts the CDROM\n\n");	
    printf("  kfmclient exec file:/home/weis/data/test.html\n"
           "             // Opens the file with default binding\n\n");
    printf("  kfmclient exec file:/home/weis/data/test.html Netscape\n"
           "             // Opens the file with netscape\n\n");
    printf("  kfmclient exec ftp://localhost/ Open\n"
           "             // Opens new window with URL\n\n");
    printf("  kfmclient exec file:/root/Desktop/emacs.desktop\n"
           "             // Starts emacs\n\n");
    printf("  kfmclient exec file:/root/Desktop/cdrom.desktop\n"
           "             // Opens the CD-ROM's mount directory\n\n");
    return 0;
  }
    
  clientApp a( argc, argv );

  KTraderServiceProvider sp;

  return a.doIt( argc, argv );
}

bool clientApp::openFileManagerWindow(const char* _url)
{
  bool ok = getKonqy();

  if (ok)
  {
    CORBA::Request_var req = m_vKonqy->_request( "createBrowserWindow" );
    
    req->add_in_arg( "url" ) <<= _url;
    
    req->result()->value()->type( OpenParts::_tc_MainWindow );
    
    req->invoke();
    
//    OpenParts::MainWindow_var m_vMainWindow = m_vKonqy->createBrowserWindow( _url );
  }
  
  return ok;
}

void clientApp::initRegistry()
{
  // Register mimetypes and services, for kpropsdlg and krun
  // no need to load the registry if we're running a local kded, since it already
  // loaded it.
  if ( !kded->isLocalKded() )
  {
    KRegistry * registry = new KRegistry;
    registry->addFactory( new KServiceTypeFactory );
    //registry->addFactory( new KServiceFactory ); we don't need that one do we ? (David)
    registry->load( );
  }    
}

int clientApp::doIt( int argc, char **argv )
{
  if ( argc < 2 )
  {
    fprintf( stderr, "Syntax Error: Too few arguments\n" );
    return 1;
  }
    
  if ( strcmp( argv[1], "refreshDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, "Syntax Error: Too many arguments\n" );
      return 1;
    }
    //	kfm.refreshDesktop();
    // obsolete
  }
  else if ( strcmp( argv[1], "sortDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, "Syntax Error: Too many arguments\n" );
      return 1;
    }
    bool ok = getKDesky();
    if (ok) 
      m_vKDesky->rearrangeIcons( (CORBA::Boolean) false /* don't ask */ );
    return ok;
  }
  else if ( strcmp( argv[1], "configure" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, "Syntax Error: Too many arguments\n" );
      return 1;
    }
    //	kfm.configure();
  }
  else if ( strcmp( argv[1], "openURL" ) == 0 )
  {
    if ( argc == 2 )
    {
      return openFileManagerWindow( QDir::homeDirPath() );
    }
    else if ( argc == 3 )
    {
      return openFileManagerWindow( argv[2] );
    }
    else
    {
      fprintf( stderr, "Syntax Error: Too many arguments\n" );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "refreshDirectory" ) == 0 )
  {
    if ( argc == 2 )
    {
      return openFileManagerWindow( QDir::homeDirPath() );
    }
    else if ( argc == 3 )
    {
      // kfm.refreshDirectory( argv[2] );
      // obsolete
    }
    else
    {
      fprintf( stderr, "Syntax Error: Too many arguments\n" );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "openProperties" ) == 0 )
  {
    if ( argc == 3 )
    {
      initRegistry();
      PropertiesDialog * p = new PropertiesDialog( argv[2] );
      QObject::connect( p, SIGNAL( propertiesClosed() ), this, SLOT( quit() ));
      exec();
    }
    else
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "exec" ) == 0 )
  {
    initRegistry(); // needed by KRun
    if ( argc == 3 )
    {
      KRun * run = new KRun( argv[2] );
      QObject::connect( run, SIGNAL( finished() ), this, SLOT( quit() ));
      QObject::connect( run, SIGNAL( error() ), this, SLOT( quit() ));
      exec();
    }
    else if ( argc == 4 )
    {
      QStringList urls;
      urls.append( argv[2] );
      KService::Ptr serv = KServiceProvider::getServiceProvider()->serviceByServiceType( argv[3] );
      if (!serv) return 1;
      bool ret = KRun::run( *serv, urls );
      if (!ret) return 1;
    }
    else
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "move" ) == 0 )
  {
    if ( argc <= 3 )
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }
    QString src = "";
    int i = 2;
    while ( i <= argc - 2 )
    {
      src += argv[i];
      if ( i < argc - 2 )
        src += "\n";
      i++;
    }
	
    // kfm.moveClient( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "copy" ) == 0 )
  {
    if ( argc <= 3 )
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }
    QString src = "";
    int i = 2;
    while ( i <= argc - 2 )
    {
      src += argv[i];
      if ( i < argc - 2 )
        src += "\n";
      i++;
    }
	
    // kfm.copy( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "folder" ) == 0 )
  {
    if ( argc <=2 )
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }

    if (argc > 3) {
      QString src = "";
      int i = 2;
      while ( i <= argc - 2 )
      {
        src += argv[i];
        if ( i < argc - 2 )
          src += "\n";
        i++;
      }
	
      // kfm.moveClient( src.data(), argv[ argc - 1 ] );
    }
    else
    {
      return openFileManagerWindow( argv[2] );
    }
  }
  else if ( strcmp( argv[1], "selectDesktopIcons" ) == 0 )
  {
    if ( argc == 7 )
    {
      int x = atoi( argv[2] );
      int y = atoi( argv[3] );	  
      int w = atoi( argv[4] );
      int h = atoi( argv[5] );
      // bool bAdd = (bool) atoi( argv[6] ); /* currently unused */
      bool ok = getKDesky();
      if (ok)
        m_vKDesky->selectIconsInRect( x, y, w, h /* , bAdd TODO */ );
      return ok;
    }
    else
    {
      fprintf( stderr, "Syntax Error: Too many/few arguments\n" );
      return 1;
    }
  }
  else
  {
    fprintf( stderr, "Syntax Error: Unknown command '%s'\n",argv[1] );
    return 1;
  }
  return 0;
}

bool clientApp::getKonqy()
{
  KTrader::OfferList offers = trader->query( "Browser", "Name == 'Konqueror'" );

  if ( offers.count() != 1 )
  {
    printf("Found %i offers\n", offers.count());
    fprintf( stderr, "Error: Can't find Konqueror service\n" );
    return false;
  }

  m_vKonqy = activator->activateService( "Konqueror", "IDL:Browser/BrowserFactory:1.0", "Konqueror" );

  if ( CORBA::is_nil( m_vKonqy ) )
  {
      fprintf( stderr, "Error: Can't connect to Konqueror\n" );
      return false;
  }

  return true;
}

bool clientApp::getKDesky()
{
  // Get naming service
  KNaming *naming = kded->knaming();

  // Lookup KDesktopIf object
  CORBA::Object_var obj = naming->resolve( "IDL:KDesktopIf:1.0" );
  if ( CORBA::is_nil( obj ) )
  {
    fprintf( stderr, "Error: Can't connect to KDesktop\n" );
    return false;
  }

  // Try to cast the object
  m_vKDesky = KDesktopIf::_narrow( obj );

  if ( CORBA::is_nil( m_vKDesky ) )
  {
    fprintf( stderr, "Error: Can't connect to KDesktop\n" );
    return false;
  }
  return true;
}
