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
#include <qstring.h>

#include <kpropsdlg.h>
#include <krun.h>
#include <klocale.h>
#include <kservice.h>
#include <ktrader.h>
#include <kprocess.h>
#include <dcopclient.h>

#include "kfmclient.h"
#include "KonquerorIface_stub.h"
#include "KDesktopIface_stub.h"

int main( int argc, char **argv )
{
  clientApp a( argc, argv, "kfmclient" );
  if ( argc == 1 )
  {
    printf(i18n("\nSyntax:\n"));
    printf(i18n("  kfmclient openURL\n"
                "            # Opens a dialog to ask you for the URL\n\n"));
    printf(i18n("  kfmclient openURL 'url'\n"
                "            # Opens a window showing 'url'. If such a window\n"));
    printf(i18n("            #   exists, it is shown. 'url' may be \"trash:/\"\n"
                "            #   to open the trash bin.\n\n"));
    printf(i18n("  kfmclient openProperties 'url'\n"
                "            # Opens a properties menu\n\n"));
    printf(i18n("  kfmclient exec ['url' ['binding']]\n"
                "            # Tries to execute 'url'. 'url' may be a usual\n"
                "            #   URL, this URL will be opened. You may omit\n"
                "            #   'binding'. In this case the default binding\n"));
    printf(i18n("            #   is tried. Of course URL may be the URL of a\n"
                "            #   document, or it may be a *.desktop file.\n"));
    printf(i18n("            #   This way you could for example mount a device\n"
                "            #   by passing 'Mount default' as binding to \n"
                "            #   'cdrom.desktop'\n\n"));
    printf(i18n("  kfmclient move 'src' 'dest'\n"
                "            # Moves the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n"));
    printf(i18n("            #   'dest' may be \"trash:/\" to move the files\n"
                "            #   in the trash bin.\n\n"));
    printf(i18n("  kfmclient copy 'src' 'dest'\n"
                "            # Copies the URL 'src' to 'dest'.\n"
                "            #   'src' may be a list of URLs.\n\n"));
    printf(i18n("  kfmclient sortDesktop\n"
                "            # Rearranges all icons on the desktop.\n\n"));
    printf(i18n("  kfmclient selectDesktopIcons x y w h add\n"
                "            # Selects the icons on the desktop in the given rectangle\n"
                "            # If add is 1, adds selection to the current one\n\n"));
    printf(i18n("  kfmclient configure\n"
                "            # Re-read konqueror's configuration.\n\n"));
    printf(i18n("  kfmclient configureDesktop\n"
                "            # Re-read kdesktop's configuration.\n\n"));
    
    printf(i18n("*** Examples:\n"
                "  kfmclient exec file:/root/Desktop/cdrom.desktop \"Mount default\"\n"
                "             // Mounts the CDROM\n\n"));	
    printf(i18n("  kfmclient exec file:/home/weis/data/test.html\n"
                "             // Opens the file with default binding\n\n"));
    printf(i18n("  kfmclient exec file:/home/weis/data/test.html Netscape\n"
                "             // Opens the file with netscape\n\n"));
    printf(i18n("  kfmclient exec ftp://localhost/\n"
                "             // Opens new window with URL\n\n"));
    printf(i18n("  kfmclient exec file:/root/Desktop/emacs.desktop\n"
                "             // Starts emacs\n\n"));
    printf(i18n("  kfmclient exec file:/root/Desktop/cdrom.desktop\n"
                "             // Opens the CD-ROM's mount directory\n\n"));
    return 0;
  }
    

  a.dcopClient()->attach();

  return a.doIt( argc, argv );
}

bool clientApp::openFileManagerWindow(const char* _url)
{
  
  if ( dcopClient()->isApplicationRegistered( "konqueror" ) )
  {
    KonquerorIface_stub konqy( "konqueror", "KonquerorIface" );
    konqy.openBrowserWindow( QString( _url ) );
  }
  else
  {
    KProcess proc;
    proc << "konqueror" << QString( _url );
    proc.start( KProcess::DontCare );
  }
  
  return true;
}

int clientApp::doIt( int argc, char **argv )
{
  if ( argc < 2 )
  {
    fprintf( stderr, i18n("Syntax Error: Too few arguments\n") );
    return 1;
  }
    
  if ( strcmp( argv[1], "openURL" ) == 0 )
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
      fprintf( stderr, i18n("Syntax Error: Too many arguments\n") );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "openProperties" ) == 0 )
  {
    if ( argc == 3 )
    {
      PropertiesDialog * p = new PropertiesDialog( argv[2] );
      QObject::connect( p, SIGNAL( propertiesClosed() ), this, SLOT( quit() ));
      exec();
    }
    else
    {
      fprintf( stderr, i18n("Syntax Error: Wrong number of arguments\n") );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "exec" ) == 0 )
  {
    if ( argc == 2 )
    {
      KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
      kdesky.popupExecuteCommand();
    }
    else if ( argc == 3 )
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
//      KService::Ptr serv = KServiceProvider::getServiceProvider()->serviceByServiceType( argv[3] );
      KService::Ptr serv = (*KTrader::self()->query( argv[ 3 ] ).begin());
      if (!serv) return 1;
      bool ret = KRun::run( *serv, urls );
      if (!ret) return 1;
    }
    else
    {
      fprintf( stderr, i18n("Syntax Error: Wrong number of arguments\n") );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "move" ) == 0 )
  {
    if ( argc <= 3 )
    {
      fprintf( stderr, i18n("Syntax Error: Too few arguments\n") );
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
    // TODO	
    // kfm.moveClient( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "copy" ) == 0 )
  {
    if ( argc <= 3 )
    {
      fprintf( stderr, i18n("Syntax Error: Too few arguments\n") );
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
    // TODO
    // kfm.copy( src.data(), argv[ argc - 1 ] );
  }
  else if ( strcmp( argv[1], "sortDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, i18n("Syntax Error: Too many arguments\n") );
      return 1;
    }
    
    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.rearrangeIcons( (int)false );

    return true;
  }
  else if ( strcmp( argv[1], "selectDesktopIcons" ) == 0 )
  {
    if ( argc == 7 )
    {
      int x = atoi( argv[2] );
      int y = atoi( argv[3] );	  
      int w = atoi( argv[4] );
      int h = atoi( argv[5] );
      // bool bAdd = (bool) atoi( argv[6] ); /* currently unused */ // TODO
      KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
      kdesky.selectIconsInRect( x, y, w, h );
    }
    else
    {
      fprintf( stderr, i18n("Syntax Error: Wrong number of arguments\n") );
      return 1;
    }
  }
  else if ( strcmp( argv[1], "configure" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, i18n("Syntax Error: Too many arguments\n") );
      return 1;
    }
    QByteArray data;
    kapp->dcopClient()->send( "*", "KonqMainViewIface", "reparseConfiguration()", data );
    // Warning. In case something is added/changed here, keep kcontrol/konq/main.cpp in sync.
  }
  else if ( strcmp( argv[1], "configureDesktop" ) == 0 )
  {
    if ( argc != 2 )
    {
      fprintf( stderr, i18n("Syntax Error: Too many arguments\n") );
      return 1;
    }

    KDesktopIface_stub kdesky( "kdesktop", "KDesktopIface" );
    kdesky.configure();
  }
  else
  {
    fprintf( stderr, i18n("Syntax Error: Unknown command '%s'\n"),argv[1] );
    return 1;
  }
  return 0;
}

